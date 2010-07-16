
/// @file libym_mvn/verilog/ReaderImpl.cc
/// @brief ReaderImpl の実装クラス
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2010 Yusuke Matsunaga
/// All rights reserved.


#include "ReaderImpl.h"
#include "DeclMap.h"
#include "ym_mvn/MvMgr.h"
#include "ym_mvn/MvModule.h"
#include "ym_mvn/MvPort.h"
#include "ym_mvn/MvNode.h"
#include "ym_verilog/vl/VlModule.h"
#include "ym_verilog/vl/VlPrimitive.h"
#include "ym_verilog/vl/VlUdp.h"
#include "ym_verilog/vl/VlIODecl.h"
#include "ym_verilog/vl/VlDecl.h"
#include "ym_verilog/vl/VlPort.h"
#include "ym_verilog/vl/VlContAssign.h"
#include "ym_verilog/vl/VlExpr.h"
#include "ym_verilog/vl/VlRange.h"


BEGIN_NAMESPACE_YM_MVN_VERILOG

// @brief コンストラクタ
ReaderImpl::ReaderImpl() :
  mVlMgr(mMsgMgr)
{
  mMvMgr = NULL;
}

// @brief デストラクタ
ReaderImpl::~ReaderImpl()
{
}

// @brief 内部のデータをクリアする．
void
ReaderImpl::clear()
{
  mVlMgr.clear();
}

// @brief verilog 形式のファイルを読み込む．
// @param[in] filename ファイル名
// @param[in] searchpath サーチパス
// @param[in] watcher_list 行番号ウォッチャーのリスト
// @retval true 正常に読み込めた．
// @retval false 読込中にエラーが起こった．
bool
ReaderImpl::read(const string& filename,
		 const SearchPathList& searchpath,
		 const list<VlLineWatcher*> watcher_list)
{
  return mVlMgr.read_file(filename, searchpath, watcher_list);
}

// @brief 今まで読み込んだ情報からネットワークを生成する．
// @param[in] mgr ネットワーク生成用のマネージャ
// @retval true 正常に処理を行った．
// @retval false 生成中にエラーが起こった．
bool
ReaderImpl::gen_network(MvMgr& mgr)
{
  using namespace nsVerilog;

  mVlMgr.elaborate();
  
  mMvMgr = &mgr;
  
  list<const VlModule*> tmp_list(mVlMgr.topmodule_list());
  for (list<const VlModule*>::const_iterator p = tmp_list.begin();
       p != tmp_list.end(); ++ p) {
    const VlModule* vl_module = *p;

    if ( vl_module->is_cell_instance() ) continue;

    // module を実体化
    MvModule* module = gen_module(vl_module);
    if ( module == NULL ) {
      return false;
    }
  }

  return true;
}

// @brief メッセージハンドラを付加する．
void
ReaderImpl::add_msg_handler(MsgHandler* msg_handler)
{
  mMsgMgr.reg_handler(msg_handler);
}

// @brief module を生成する．
// @param[in] vl_module 対象のモジュール
MvModule*
ReaderImpl::gen_module(const nsVerilog::VlModule* vl_module)
{
  using namespace nsVerilog;
  
  // ポート数，入出力のビット幅を調べる．
  ymuint np = vl_module->port_num();
  ymuint nio = vl_module->io_num();
  ymuint ni = 0;
  ymuint no = 0;
  for (ymuint i = 0; i < nio; ++ i) {
    const VlIODecl* io = vl_module->io(i);
    switch ( io->direction() ) {
    case kVpiInput:  ++ ni; break;
    case kVpiOutput: ++ no; break;
    default:
      mMsgMgr.put_msg(__FILE__, __LINE__,
		      io->file_region(),
		      kMsgError,
		      "MVN_VL01",
		      "Only Input/Output types are supported");
      return NULL;
    }
  }
  vector<ymuint> ibw_array(ni);
  vector<ymuint> obw_array(no);
  ni = 0;
  no = 0;
  for (ymuint i = 0; i < nio; ++ i) {
    const VlIODecl* io = vl_module->io(i);
    switch ( io->direction() ) {
    case kVpiInput:  ibw_array[ni] = io->bit_size(); ++ ni; break;
    case kVpiOutput: obw_array[no] = io->bit_size(); ++ no; break;
    default: break;
    }
  }
  
  MvModule* module = mMvMgr->new_module(vl_module->name(),
					np, ibw_array, obw_array);

  // 入出力ノードの対応表を作る．
  DeclMap decl_map;
  ymuint i1 = 0;
  ymuint i2 = 0;
  for (ymuint i = 0; i < nio; ++ i) {
    const VlIODecl* io = vl_module->io(i);
    switch ( io->direction() ) {
    case kVpiInput:
      decl_map.add(io->decl(), module->input(i1));
      ++ i1;
      break;

    case kVpiOutput:
      decl_map.add(io->decl(), module->output(i2));
      ++ i2;
      break;

    default:
      break;
    }
  }
  
  // ポートの接続を行う．
  for (ymuint i = 0; i < np; ++ i) {
    const VlPort* port = vl_module->port(i);
    const VlExpr* expr = port->low_conn();
    if ( expr->type() == kVpiOperation ) {
      assert_cond( expr->op_type() == kVpiConcatOp, __FILE__, __LINE__);
      ymuint n = expr->operand_num();
      mMvMgr->init_port(module, i, port->name(), n);
      for (ymuint j = 0; j < n; ++ j) {
	MvNode* node;
	ymuint msb;
	ymuint lsb;
	switch ( gen_portref(expr->operand(j), decl_map, node, msb, lsb) ) {
	case 0:
	  mMvMgr->set_port_ref(module, i, j, node);
	  break;
	  
	case 1:
	  mMvMgr->set_port_ref(module, i, j, node, msb);
	  break;
	  
	case 2:
	  mMvMgr->set_port_ref(module, i, j, node, msb, lsb);
	  break;
	  
	default:
	  assert_not_reached(__FILE__, __LINE__);
	  break;
	}
      }
    }
    else {
      mMvMgr->init_port(module, i, port->name(), 1);
      MvNode* node;
      ymuint msb;
      ymuint lsb;
      switch ( gen_portref(expr, decl_map, node, msb, lsb) ) {
      case 0:
	mMvMgr->set_port_ref(module, i, 0, node);
	break;

      case 1:
	mMvMgr->set_port_ref(module, i, 0, node, msb);
	break;

      case 2:
	mMvMgr->set_port_ref(module, i, 0, node, msb, lsb);
	break;

      default:
	assert_not_reached(__FILE__, __LINE__);
	break;
      }
    }
  }

  // 宣言要素を生成する．
  bool stat = gen_decl(module, decl_map, vl_module);
  if ( !stat ) {
    return NULL;
  }

  // 要素を生成する．
  stat = gen_item(module, decl_map, vl_module);
  if ( !stat ) {
    return NULL;
  }
  
  return module;
}

// @brief 宣言要素を生成する．
// @param[in] module モジュール
// @param[in] decl_map 宣言要素とノードの対応表
// @param[in] vl_scope 対象のスコープ
// @retval true 成功した．
// @retval false エラーが起こった．
bool
ReaderImpl::gen_decl(MvModule* module,
		     DeclMap& decl_map,
		     const nsVerilog::VlNamedObj* vl_scope)
{
  using namespace nsVerilog;

  // ネットの生成
  {
    vector<const VlDecl*> net_list;
    if ( mVlMgr.find_decl_list(vl_scope, vpiNet, net_list) ) {
      for (vector<const VlDecl*>::iterator p = net_list.begin();
	   p != net_list.end(); ++ p) {
	const VlDecl* vl_decl = *p;
	// 仮の through ノードに対応させる．
	ymuint bw = vl_decl->bit_size();
	MvNode* node = mMvMgr->new_through(module, bw);
	decl_map.add(vl_decl, node);
      }
    }
  }

  // ネット配列の生成
  {
    vector<const VlDecl*> netarray_list;
    if ( mVlMgr.find_decl_list(vl_scope, vpiNetArray, netarray_list) ) {
      for (vector<const VlDecl*>::iterator p = netarray_list.begin();
	   p != netarray_list.end(); ++ p) {
	const VlDecl* vl_decl = *p;
	ymuint d = vl_decl->dimension();
	ymuint array_size = 1;
	for (ymuint i = 0; i < d; ++ i) {
	  array_size *= vl_decl->range(i)->size();
	}
	for (ymuint i = 0; i < array_size; ++ i) {
	  // 仮の through ノードに対応させる．
	  ymuint bw = vl_decl->bit_size();
	  MvNode* node = mMvMgr->new_through(module, bw);
	  decl_map.add(vl_decl, i, node);
	}
      }
    }
  }

  // 内部スコープ要素の生成
  {
    vector<const VlNamedObj*> scope_list;
    if ( mVlMgr.find_genblock_list(vl_scope, scope_list) ) {
      for (vector<const VlNamedObj*>::iterator p = scope_list.begin();
	   p != scope_list.end(); ++ p) {
	const VlNamedObj* vl_scope1 = *p;
	bool stat = gen_decl(module, decl_map, vl_scope1);
	if ( !stat ) {
	  return false;
	}
      }
    }
  }
  
  return true;
}

// @brief 要素を生成する．
// @param[in] module モジュール
// @param[in] decl_map 宣言要素とノードの対応表
// @param[in] vl_scope 対象のスコープ
// @retval true 成功した．
// @retval false エラーが起こった．
bool
ReaderImpl::gen_item(MvModule* module,
		     DeclMap& decl_map,
		     const nsVerilog::VlNamedObj* vl_scope)
{
  using namespace nsVerilog;

  // モジュールインスタンスの生成
  {
    vector<const VlModule*> module_list;
    if ( mVlMgr.find_module_list(vl_scope, module_list) ) {
      for (vector<const VlModule*>::iterator p = module_list.begin();
	   p != module_list.end(); ++ p) {
	const VlModule* vl_module = *p;
	gen_moduleinst(vl_module, module, decl_map);
      }
    }
  }

  // モジュール配列インスタンスの生成
  {
    vector<const VlModuleArray*> modulearray_list;
    if ( mVlMgr.find_modulearray_list(vl_scope, modulearray_list) ) {
      for (vector<const VlModuleArray*>::iterator p = modulearray_list.begin();
	   p != modulearray_list.end(); ++ p) {
	const VlModuleArray* vl_module_array = *p;
	ymuint n = vl_module_array->elem_num();
	for (ymuint i = 0; i < n; ++ i) {
	  const VlModule* vl_module = vl_module_array->elem_by_offset(i);
	  gen_moduleinst(vl_module, module, decl_map);
	}
      }
    }
  }

  // プリミティブインスタンスの生成
  {
    vector<const VlPrimitive*> primitive_list;
    if ( mVlMgr.find_primitive_list(vl_scope, primitive_list) ) {
      for (vector<const VlPrimitive*>::iterator p = primitive_list.begin();
	   p != primitive_list.end(); ++ p) {
	const VlPrimitive* vl_prim = *p;
	gen_priminst(vl_prim, module, decl_map);
      }
    }
  }
  
  // プリミティブ配列インスタンスの生成
  {
    vector<const VlPrimArray*> primarray_list;
    if ( mVlMgr.find_primarray_list(vl_scope, primarray_list) ) {
      for (vector<const VlPrimArray*>::iterator p = primarray_list.begin();
	   p != primarray_list.end(); ++ p) {
	const VlPrimArray* vl_primarray = *p;
	ymuint n = vl_primarray->elem_num();
	for (ymuint i = 0; i < n; ++ i) {
	  const VlPrimitive* vl_prim = vl_primarray->elem_by_offset(i);
	  gen_priminst(vl_prim, module, decl_map);
	}
      }
    }
  }

  // 継続的代入文の生成
  {
    vector<const VlContAssign*> contassign_list;
    if ( mVlMgr.find_contassign_list(vl_scope, contassign_list) ) {
      for (vector<const VlContAssign*>::iterator p = contassign_list.begin();
	   p != contassign_list.end(); ++ p) {
	const VlContAssign* vl_contassign = *p;
	const VlExpr* lhs = vl_contassign->lhs();
	const VlExpr* rhs = vl_contassign->rhs();
	MvNode* node = gen_expr1(module, rhs, decl_map);
	connect_lhs(module, lhs, node, decl_map);
      }
    }
  }

  // 内部スコープ要素の生成
  {
    vector<const VlNamedObj*> scope_list;
    if ( mVlMgr.find_genblock_list(vl_scope, scope_list) ) {
      for (vector<const VlNamedObj*>::iterator p = scope_list.begin();
	   p != scope_list.end(); ++ p) {
	const VlNamedObj* vl_scope1 = *p;
	bool stat = gen_item(module, decl_map, vl_scope1);
	if ( !stat ) {
	  return false;
	}
      }
    }
  }
  
  return true;
}

// @brief portref の実体化を行う．
// @param[in] expr 対象の式
// @param[in] decl_map 入出力ノードの対応表
// @param[out] node 対応するノードを格納する変数
// @param[out] msb ビット指定位置か範囲指定の MSB を格納する変数
// @param[out] lsb 範囲指定の LSB を格納する変数
// @retval 0 単純な形式だった．
// @retval 1 ビット指定形式だった．
// @retval 2 範囲指定形式だった．
int
ReaderImpl::gen_portref(const VlExpr* expr,
			const DeclMap& decl_map,
			MvNode*& node,
			ymuint& msb,
			ymuint& lsb)
{
  const VlDecl* decl = expr->decl_obj();
  assert_cond( decl != NULL, __FILE__, __LINE__);
  node = decl_map.get(decl);
  if ( node == NULL ) {
    cout << decl->full_name() << " is not found in decl_map" << endl;
  }
  assert_cond( node != NULL, __FILE__, __LINE__);

  switch ( expr->type() ) {
  case kVpiBitSelect:
    assert_cond( node != NULL, __FILE__, __LINE__);
    assert_cond( expr->is_constant_select(), __FILE__, __LINE__);
    assert_cond( expr->declarray_dimension() == 0, __FILE__, __LINE__);
    msb = expr->index_val();
    return 1;

  case kVpiPartSelect:
    assert_cond( node != NULL, __FILE__, __LINE__);
    assert_cond( expr->is_constant_select(), __FILE__, __LINE__);
    assert_cond( expr->declarray_dimension() == 0, __FILE__, __LINE__);
    msb = expr->left_range_val();
    lsb = expr->right_range_val();
    return 2;

  default:
    break;
  }
  return 0;
}

// @brief モジュールインスタンスの生成を行う．
// @param[in] vl_module モジュール
// @param[in] parent_module 親のモジュール
// @param[in] decl_map 宣言要素の対応表
// @return 対応するノードを返す．
MvNode*
ReaderImpl::gen_moduleinst(const VlModule* vl_module,
			   MvModule* parent_module,
			   const DeclMap& decl_map)
{
  if ( strcmp(vl_module->def_name(), "GTECH_FD2") == 0 ) {
    // GTECH_FD2( input D, input CP, input CD, output Q );
    //   D:  データ入力
    //   CP: クロック
    //   CD: 非同期リセット
    //   Q:  データ出力
    MvNode* node = mMvMgr->new_dff1(parent_module, 1);
    ymuint np = vl_module->port_num();
    assert_cond( np == 5, __FILE__, __LINE__);
    for (ymuint i = 0; i < 5; ++ i) {
      const VlPort* vl_port = vl_module->port(i);
      const char* port_name = vl_port->name();
      const VlExpr* expr = vl_port->high_conn();
      if ( strcmp(port_name, "D") == 0 ) {
	MvNode* node1 = gen_expr1(parent_module, expr, decl_map);
	mMvMgr->connect(node1, 0, node, 0);
      }
      else if ( strcmp(port_name, "CP") == 0 ) {
	MvNode* node1 = gen_expr1(parent_module, expr, decl_map);
	mMvMgr->connect(node1, 0, node, 1);
      }
      else if ( strcmp(port_name, "CD") == 0 ) {
	MvNode* node1 = gen_expr1(parent_module, expr, decl_map);
	mMvMgr->connect(node1, 0, node, 2);
      }
      else if ( strcmp(port_name, "Q") == 0 ) {
	connect_lhs(parent_module, expr, node, decl_map);
      }
      else if ( strcmp(port_name, "QN") == 0 ) {
	if ( expr != NULL ) {
	  MvNode* node1 = mMvMgr->new_not(parent_module, 1);
	  mMvMgr->connect(node, 0, node1, 0);
	  connect_lhs(parent_module, expr, node1, decl_map);
	}
      }
      else {
	assert_not_reached(__FILE__, __LINE__);
      }
    }
    return node;
  }
  
  MvModule* module = gen_module(vl_module);
  if ( module == NULL ) {
    return NULL;
  }
  MvNode* node = mMvMgr->new_inst(parent_module, module);
  ymuint np = vl_module->port_num();
  for (ymuint i = 0; i < np; ++ i) {
    const VlPort* vl_port = vl_module->port(i);
    MvPort* port = module->port(i);

    const VlExpr* hi = vl_port->high_conn();
    switch ( vl_port->direction() ) {
    case kVpiInput:
      // 普通の式が書ける．
      {
	MvNode* node = gen_expr1(parent_module, hi, decl_map);
	connect_port1(parent_module, port, node);
      }
      break;
      
    case kVpiOutput:
      {
	MvNode* node = port_to_node(parent_module, port);
	connect_lhs(parent_module, hi, node, decl_map);
      }
      break;
      
    case kVpiMixedIO:
      // 単純な参照か連結のみ
      //connect_port2(port, hi);
      break;
      
    default:
      assert_not_reached(__FILE__, __LINE__);
      break;
    }
  }

  return node;
}

// @brief ポートに接続する．
// @param[in] parent_module 親のモジュール
// @param[in] port 対象のポート
// @param[in] node 接続するノード
void
ReaderImpl::connect_port1(MvModule* parent_module,
			  const MvPort* port,
			  MvNode* node)
{
  ymuint n = port->port_ref_num();
  ymuint bit_width = static_cast<const MvNode*>(node)->output(0)->bit_width();
  if ( n == 1 ) {
    const MvPortRef* port_ref = port->port_ref(0);
    assert_cond( port_ref->bit_width() == bit_width, __FILE__, __LINE__);
    if ( port_ref->is_simple() ) {
      mMvMgr->connect(node, 0, port_ref->node(), 0);
    }
    else if ( port_ref->has_bitselect() ) {
      // 未完
    }
    else if ( port_ref->has_partselect() ) {
      // 未完
    }
  }
  else {
    ymuint last = 0;
    for (ymuint i = 0; i < n; ++ i) {
      const MvPortRef* port_ref = port->port_ref(i);
      ymuint bw = port_ref->bit_width();
      MvNode* node1 = NULL;
      if ( bw == 1 ) {
	node1 = mMvMgr->new_constbitselect(parent_module,
					   last,
					   bit_width);
      }
      else {
	node1 = mMvMgr->new_constpartselect(parent_module,
					    last + bw - 1,
					    last,
					    bit_width);
      }
      mMvMgr->connect(node, 0, node1, 0);
      last += bw;
      if ( port_ref->is_simple() ) {
	mMvMgr->connect(node1, 0, port_ref->node(), 0);
      }
      else if ( port_ref->has_bitselect() ) {
	// 未完
      }
      else if ( port_ref->has_partselect() ) {
	// 未完
      }
    }
  }
}

// @brief ポートの内容に対応するノードを作る．
// @param[in] parent_module 親のモジュール
// @param[in] port 対象のポート
MvNode*
ReaderImpl::port_to_node(MvModule* parent_module,
			 const MvPort* port)
{
  ymuint n = port->port_ref_num();
  if ( n == 1 ) {
    const MvPortRef* port_ref = port->port_ref(0);
    MvNode* node = port_ref_to_node(parent_module, port_ref);
    return node;
  }
  
  vector<ymuint> bw_array(n);
  for (ymuint i = 0; i < n; ++ i) {
    const MvPortRef* port_ref = port->port_ref(i);
    bw_array[i] = port_ref->bit_width();
  }
  MvNode* node_o = mMvMgr->new_concat(parent_module, bw_array);
  for (ymuint i = 0; i < n; ++ i) {
    const MvPortRef* port_ref = port->port_ref(i);
    MvNode* node = port_ref_to_node(parent_module, port_ref);
    mMvMgr->connect(node, 0, node_o, i);
  }
  return node_o;
}

// @brief ポート参照式に対応するノードを作る．
// @param[in] parent_module 親のモジュール
// @param[in] port_ref 対象のポート参照式
MvNode*
ReaderImpl::port_ref_to_node(MvModule* parent_module,
			     const MvPortRef* port_ref)
{
  MvNode* node = port_ref->node();
  ymuint bw = static_cast<const MvNode*>(node)->output(0)->bit_width();
  MvNode* node1 = NULL;
  if ( port_ref->is_simple() ) {
    node1 = node;
  }
  else if ( port_ref->has_bitselect() ) {
    node1 = mMvMgr->new_constbitselect(parent_module,
				       port_ref->bitpos(),
				       bw);
    mMvMgr->connect(node, 0, node1, 0);
  }
  else if ( port_ref->has_partselect() ) {
    MvNode* node1 = mMvMgr->new_constpartselect(parent_module,
						port_ref->msb(),
						port_ref->lsb(),
						bw);
    mMvMgr->connect(node, 0, node1, 0);
  }
  else {
    assert_not_reached(__FILE__, __LINE__);
  }
  return node1;
}

// @brief 左辺式に接続する．
// @param[in] parent_module 親のモジュール
// @param[in] expr 左辺式
// @param[in] node 右辺に対応するノード
// @param[in] decl_map 宣言要素の対応表
void
ReaderImpl::connect_lhs(MvModule* parent_module,
			const VlExpr* expr,
			MvNode* node,
			const DeclMap& decl_map)
{
  switch ( expr->type() ) {
  case kVpiBitSelect:
    // 未完
    break;
    
  case kVpiPartSelect:
    // 未完
    break;

  case kVpiOperation:
    assert_cond( expr->op_type() == kVpiConcatOp, __FILE__, __LINE__);
    {
      ymuint n = expr->operand_num();
      ymuint offset = 0;
      for (ymuint i = 0; i < n; ++ i) {
	const VlExpr* expr1 = expr->operand(i);
	connect_lhs_sub(parent_module, expr1, node, decl_map, offset);
	offset += expr1->bit_size();
      }
    }
    break;

  default:
    {
      MvNode* node1 = gen_expr2(expr, decl_map);
      mMvMgr->connect(node, 0, node1, 0);
    }
    break;
  }
}

// @brief 左辺式に接続する．
// @param[in] parent_module 親のモジュール
// @param[in] expr 左辺式
// @param[in] node 右辺に対応するノード
// @param[in] decl_map 宣言要素の対応表
void
ReaderImpl::connect_lhs_sub(MvModule* parent_module,
			    const VlExpr* expr,
			    MvNode* node,
			    const DeclMap& decl_map,
			    ymuint offset)
{
  switch ( expr->type() ) {
  case kVpiBitSelect:
    // 未完
    break;
    
  case kVpiPartSelect:
    // 未完
    break;

  case kVpiOperation:
    assert_cond( expr->op_type() == kVpiConcatOp, __FILE__, __LINE__);
    {
      ymuint n = expr->operand_num();
      ymuint offset1 = 0;
      for (ymuint i = 0; i < n; ++ i) {
	const VlExpr* expr1 = expr->operand(i);
	connect_lhs_sub(parent_module, expr1, node, decl_map, offset + offset1);
	offset1 += expr1->bit_size();
      }
    }
    break;

  default:
    {
      MvNode* node1 = gen_expr2(expr, decl_map);
      ymuint bw = static_cast<const MvNode*>(node1)->input(0)->bit_width();
      MvNode* node2 = NULL;
      if ( bw == 1 ) {
	node2 = mMvMgr->new_constbitselect(parent_module,
					   offset,
					   bw);
      }
      else {
	node2 = mMvMgr->new_constpartselect(parent_module,
					    offset + bw - 1,
					    offset,
					    bw);
      }
      mMvMgr->connect(node, 0, node2, 0);
      mMvMgr->connect(node2, 0, node1, 0);
    }
    break;
  }
}

// @brief プリミティブインスタンスの生成を行う．
// @param[in] prim プリミティブ
// @param[in] parent_module 親のモジュール
// @param[in] decl_map 宣言要素の対応表
void
ReaderImpl::gen_priminst(const VlPrimitive* prim,
			 MvModule* parent_module,
			 const DeclMap& decl_map)
{
  ymuint nt = prim->port_num();
  ymuint ni = 0;
  ymuint no = 0;
  if ( prim->prim_type() == kVpiBufPrim ) {
    ni = 1;
    no = nt - 1;
  }
  else {
    ni = nt - 1;
    no = 1;
  }

  vector<pair<MvNode*, ymuint> > inputs(ni);
  vector<MvNode*> outputs(no);

  switch ( prim->prim_type() ) {
  case kVpiBufPrim:
    {
      MvNode* node = mMvMgr->new_through(parent_module, 1);
      inputs[0] = make_pair(node, 0);
      for (ymuint i = 0; i < no; ++ i) {
	outputs[i] = node;
      }
    }
    break;

  case kVpiNotPrim:
    {
      MvNode* node = mMvMgr->new_not(parent_module, 1);
      inputs[0] = make_pair(node, 0);
      outputs[0] = node;
    }
    break;

  case kVpiAndPrim:
    {
      MvNode* node = gen_andtree(parent_module, ni, inputs, 0);
      outputs[0] = node;
    }
    break;
    
  case kVpiNandPrim:
    {
      MvNode* node = gen_andtree(parent_module, ni, inputs, 0);
      MvNode* node1 = mMvMgr->new_not(parent_module, 1);
      mMvMgr->connect(node, 0, node1, 0);
      outputs[0] = node1;
    }
    break;
    
  case kVpiOrPrim:
    {
      MvNode* node = gen_ortree(parent_module, ni, inputs, 0);
      outputs[0] = node;
    }
    break;
    
  case kVpiNorPrim:
    {
      MvNode* node = gen_ortree(parent_module, ni, inputs, 0);
      MvNode* node1 = mMvMgr->new_not(parent_module, 1);
      mMvMgr->connect(node, 0, node1, 0);
      outputs[0] = node1;
    }
    break;
    
  case kVpiXorPrim:
    {
      MvNode* node = gen_xortree(parent_module, ni, inputs, 0);
      outputs[0] = node;
    }
    break;
    
  case kVpiXnorPrim:
    {
      MvNode* node = gen_xortree(parent_module, ni, inputs, 0);
      MvNode* node1 = mMvMgr->new_not(parent_module, 1);
      mMvMgr->connect(node, 0, node1, 0);
      outputs[0] = node1;
    }
    break;
    
  case kVpiCombPrim:
    {
      const VlUdpDefn* udp = prim->udp_defn();
      ymuint ni = udp->port_num() - 1;
      MvNode* node = mMvMgr->new_combudp(parent_module, ni);
      for (ymuint i = 0; i < ni; ++ i) {
	inputs[i] = make_pair(node, i);
      }
      outputs[0] = node;
    }
    break;

  case kVpiSeqPrim:
    {
      const VlUdpDefn* udp = prim->udp_defn();
      ymuint ni = udp->port_num() - 1;
      MvNode* node = mMvMgr->new_sequdp(parent_module, ni);
      for (ymuint i = 0; i < ni; ++ i) {
	inputs[i] = make_pair(node, i);
      }
      outputs[0] = node;
    }
    break;

  default:
    assert_not_reached(__FILE__, __LINE__);
    break;
  }

  ymuint pos = 0;
  for (ymuint i = 0; i < no; ++ i) {
    const VlPrimTerm* term = prim->prim_term(pos);
    ++ pos;
    const VlExpr* expr = term->expr();
    connect_lhs(parent_module, expr, outputs[i], decl_map);
  }
  for (ymuint i = 0; i < ni; ++ i) {
    const VlPrimTerm* term = prim->prim_term(pos);
    ++ pos;
    const VlExpr* expr = term->expr();
    MvNode* node = gen_expr1(parent_module, expr, decl_map);
    const pair<MvNode*, ymuint>& p = inputs[i];
    mMvMgr->connect(node, 0, p.first, p.second);
  }
}

// @brief AND のバランス木を作る．
// @param[in] parent_module 親のモジュール
// @param[in] ni 入力数
// @param[in] inputs 入力ピンを格納する配列
// @param[in] offset inputs のオフセット
MvNode*
ReaderImpl::gen_andtree(MvModule* parent_module,
			ymuint ni,
			vector<pair<MvNode*, ymuint> >& inputs,
			ymuint offset)
{
  assert_cond( ni > 1, __FILE__, __LINE__);

  if ( ni == 2 ) {
    MvNode* node = mMvMgr->new_and(parent_module, 1);
    inputs[offset + 0] = make_pair(node, 0);
    inputs[offset + 1] = make_pair(node, 1);
    return node;
  }

  if ( ni == 3 ) {
    MvNode* node0 = gen_andtree(parent_module, 2, inputs, offset);
    MvNode* node = mMvMgr->new_and(parent_module, 1);
    mMvMgr->connect(node0, 0, node, 0);
    inputs[offset + 2] = make_pair(node, 1);
    return node;
  }

  ymuint nr = ni / 2;
  ymuint nl = ni - nr;

  MvNode* node0 = gen_andtree(parent_module, nl, inputs, offset);
  MvNode* node1 = gen_andtree(parent_module, nr, inputs, offset + nl);

  MvNode* node = mMvMgr->new_and(parent_module, 1);
  mMvMgr->connect(node0, 0, node, 0);
  mMvMgr->connect(node1, 0, node, 1);

  return node;
}

// @brief OR のバランス木を作る．
// @param[in] parent_module 親のモジュール
// @param[in] ni 入力数
// @param[in] inputs 入力ピンを格納する配列
// @param[in] offset inputs のオフセット
MvNode*
ReaderImpl::gen_ortree(MvModule* parent_module,
		       ymuint ni,
		       vector<pair<MvNode*, ymuint> >& inputs,
		       ymuint offset)
{
  assert_cond( ni > 1, __FILE__, __LINE__);

  if ( ni == 2 ) {
    MvNode* node = mMvMgr->new_or(parent_module, 1);
    inputs[offset + 0] = make_pair(node, 0);
    inputs[offset + 1] = make_pair(node, 1);
    return node;
  }

  if ( ni == 3 ) {
    MvNode* node0 = gen_ortree(parent_module, 2, inputs, offset);
    MvNode* node = mMvMgr->new_or(parent_module, 1);
    mMvMgr->connect(node0, 0, node, 0);
    inputs[offset + 2] = make_pair(node, 1);
    return node;
  }

  ymuint nr = ni / 2;
  ymuint nl = ni - nr;

  MvNode* node0 = gen_ortree(parent_module, nl, inputs, offset);
  MvNode* node1 = gen_ortree(parent_module, nr, inputs, offset + nl);

  MvNode* node = mMvMgr->new_or(parent_module, 1);
  mMvMgr->connect(node0, 0, node, 0);
  mMvMgr->connect(node1, 0, node, 1);

  return node;
}

// @brief XOR のバランス木を作る．
// @param[in] parent_module 親のモジュール
// @param[in] ni 入力数
// @param[in] inputs 入力ピンを格納する配列
// @param[in] offset inputs のオフセット
MvNode*
ReaderImpl::gen_xortree(MvModule* parent_module,
			ymuint ni,
			vector<pair<MvNode*, ymuint> >& inputs,
			ymuint offset)
{
  assert_cond( ni > 1, __FILE__, __LINE__);

  if ( ni == 2 ) {
    MvNode* node = mMvMgr->new_xor(parent_module, 1);
    inputs[offset + 0] = make_pair(node, 0);
    inputs[offset + 1] = make_pair(node, 1);
    return node;
  }

  if ( ni == 3 ) {
    MvNode* node0 = gen_xortree(parent_module, 2, inputs, offset);
    MvNode* node = mMvMgr->new_xor(parent_module, 1);
    mMvMgr->connect(node0, 0, node, 0);
    inputs[offset + 2] = make_pair(node, 1);
    return node;
  }

  ymuint nr = ni / 2;
  ymuint nl = ni - nr;

  MvNode* node0 = gen_xortree(parent_module, nl, inputs, offset);
  MvNode* node1 = gen_xortree(parent_module, nr, inputs, offset + nl);

  MvNode* node = mMvMgr->new_xor(parent_module, 1);
  mMvMgr->connect(node0, 0, node, 0);
  mMvMgr->connect(node1, 0, node, 1);

  return node;
}

// @brief 式に対応したノードの木を作る．
// @param[in] parent_module 親のモジュール
// @param[in] expr 式
// @param[in] decl_map 宣言要素の対応表
MvNode*
ReaderImpl::gen_expr1(MvModule* parent_module,
		      const VlExpr* expr,
		      const DeclMap& decl_map)
{
  switch ( expr->type() ) {
  case kVpiBitSelect:
    {
      MvNode* node = gen_expr2(expr, decl_map);
      if ( expr->is_constant_select() ) {
	ymuint bitpos = expr->index_val();
	const MvOutputPin* pin = static_cast<const MvNode*>(node)->output(0);
	MvNode* node1 = mMvMgr->new_constbitselect(parent_module,
						   bitpos,
						   pin->bit_width());
	mMvMgr->connect(node, 0, node1, 0);
	return node1;
      }
      else {
	MvNode* node1 = gen_expr1(parent_module, expr->index(), decl_map);
	const MvOutputPin* pin0 = static_cast<const MvNode*>(node)->output(0);
	const MvOutputPin* pin1 = static_cast<const MvNode*>(node1)->output(0);
	MvNode* node2 = mMvMgr->new_bitselect(parent_module,
					      pin0->bit_width(),
					      pin1->bit_width());
	mMvMgr->connect(node, 0, node2, 0);
	mMvMgr->connect(node1, 0, node2, 1);
	return node2;
      }
    }
    break;

  case kVpiPartSelect:
    {
      MvNode* node = gen_expr2(expr, decl_map);
      if ( expr->is_constant_select() ) {
	ymuint msb = expr->left_range_val();
	ymuint lsb = expr->right_range_val();
	const MvOutputPin* pin = static_cast<const MvNode*>(node)->output(0);
	MvNode* node1 = mMvMgr->new_constpartselect(parent_module,
						    msb, lsb,
						    pin->bit_width());
	mMvMgr->connect(node, 0, node1, 0);
	return node1;
      }
      else {
	// まだできてない．
	// というか可変 part_select は VPI がおかしいと思う．
	return NULL;
      }
    }
    break;

  case kVpiOperation:
    {
    }
    break;

  case kVpiConstant:
    {
      
    }
    break;

  default:
    return gen_expr2(expr, decl_map);
  }
  return NULL;
}

// @brief 宣言要素への参照に対応するノードを作る．
// @param[in] expr 式
// @param[in] decl_map 宣言要素の対応表
MvNode*
ReaderImpl::gen_expr2(const VlExpr* expr,
		      const DeclMap& decl_map)
{
  const VlDecl* decl = expr->decl_obj();
  assert_cond( decl != NULL, __FILE__, __LINE__);
  ymuint dim = expr->declarray_dimension();
  assert_cond( decl->dimension() == dim, __FILE__, __LINE__);
  if ( dim > 1 ) {
    // 配列型
    ymuint offset = 0;
    ymuint mlt = 1;
    for (ymuint i = 0; i < dim; ++ i) {
      const VlExpr* index = expr->declarray_index(i);
      int index_val;
      bool stat = index->eval_int(index_val);
      assert_cond( stat, __FILE__, __LINE__);
      offset += index_val * mlt;
      mlt *= decl->range(i)->size();
    }
    MvNode* node = decl_map.get(decl, offset);
    if ( node == NULL ) {
      cout << decl->name() << " is not found in decl_map" << endl;
    }
    assert_cond( node != NULL, __FILE__, __LINE__);
    return node;
  }
  else {
    MvNode* node = decl_map.get(decl);
    if ( node == NULL ) {
      cout << decl->name() << " is not found in decl_map" << endl;
    }
    assert_cond( node != NULL, __FILE__, __LINE__);
    return node;
  }
}

END_NAMESPACE_YM_MVN_VERILOG