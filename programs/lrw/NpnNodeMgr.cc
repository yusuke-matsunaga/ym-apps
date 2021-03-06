﻿
/// @file NpnNodeMgr.cc
/// @brief NpnNodeMgr の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2012 Yusuke Matsunaga
/// All rights reserved.


#include "NpnNodeMgr.h"
#include "NpnNode.h"


BEGIN_NAMESPACE_YM

BEGIN_NONAMESPACE

struct Npn4Cannon
{
  ymuint16 mFunc;
  ymuint16 mPerm;
};

// 4入力の準正規形関数への変換を表す配列
Npn4Cannon npn4norm[] = {
#include "npn4norm.h"
};

// 関数を表示する．
void
print_func(ostream& s,
	   ymuint16 func)
{
  s << hex << setw(4) << setfill('0') << func << dec << endl;
}

END_NONAMESPACE


//////////////////////////////////////////////////////////////////////
// クラス NpnNodeMgr
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
NpnNodeMgr::NpnNodeMgr() :
  mAlloc(1024),
  mHashTable(NULL),
  mHashSize(0)
{
  alloc_table(1024);

  // 定数ノードの生成
  mConstNode = alloc_node();
  mConstNode->mFunc = 0x0000U;
  mConstNode->mSupVect = 0U;
  mConstNode->mXorSupVect = 0U;
  mConstNode->mSlink = NULL;
  ASSERT_COND( mConstNode->id() == 0 );

  // 入力ノードの生成
  mInputNode = alloc_node();
  mInputNode->mId |= 1U;
  mInputNode->mFunc = 0xAAAAU;
  mInputNode->mSupVect = 1U;
  mInputNode->mXorSupVect = 1U;
  mInputNode->mSlink = NULL;
  mInputNode->mIdentList.push_back(NpnXform());
  ASSERT_COND( mInputNode->id() == 1 );
}

// @brief デストラクタ
NpnNodeMgr::~NpnNodeMgr()
{
  delete [] mHashTable;
  // ノードのメモリは mAlloc のデストラクタで無事開放される．
}

// @brief 定数0ノードを生成する．
NpnHandle
NpnNodeMgr::make_const0()
{
  return NpnHandle(0, NpnXform());
}

// @brief 定数1ノードを生成する．
NpnHandle
NpnNodeMgr::make_const1()
{
  // ちょっとトリッキーなコード
  return NpnHandle(0, NpnXform(0, 1U));
}

// @brief 入力ノードを生成する．
  /// @param[in] input_id ( 0 <= input_id < 4 )
// @note すでに同じ入力番号のノードが存在していたらそれを返す．
NpnHandle
NpnNodeMgr::make_input(ymuint input_id)
{
  switch ( input_id ) {
  case 0: return NpnHandle(1, NpnXform());
  case 1: return NpnHandle(1, NpnXform(6, 0U));
  case 2: return NpnHandle(1, NpnXform(12, 0U));
  case 3: return NpnHandle(1, NpnXform(18, 0U));
  }
  ASSERT_NOT_REACHED;
  return NpnHandle();
}

// @brief ANDノードを生成する．
// @param[in] fanin0 ファンイン0
// @param[in] fanin1 ファンイン1
// @note もしも既に同じ構造のノードがあればそれを返す．
// @note 場合によってはファンインその物や定数ノードを返すこともある．
NpnHandle
NpnNodeMgr::make_and(NpnHandle fanin0,
		     NpnHandle fanin1)
{
  return new_node(false, false, fanin0, fanin1);
}

// @brief ORノードを生成する．
// @param[in] fanin0 ファンイン0
// @param[in] fanin1 ファンイン1
// @note もしも既に同じ構造のノードがあればそれを返す．
// @note 場合によってはファンインその物や定数ノードを返すこともある．
// @note 実際には AND ノード＋反転属性
NpnHandle
NpnNodeMgr::make_or(NpnHandle fanin0,
		    NpnHandle fanin1)
{
  // AND の双対
  return ~make_and(~fanin0, ~fanin1);
}

// @brief XORノードを生成する．
// @param[in] fanin0 ファンイン0
// @param[in] fanin1 ファンイン1
// @note もしも既に同じ構造のノードがあればそれを返す．
// @note 場合によってはファンインその物や定数ノードを返すこともある．
NpnHandle
NpnNodeMgr::make_xor(NpnHandle fanin0,
		     NpnHandle fanin1)
{
  bool inv0 = fanin0.oinv();
  bool inv1 = fanin1.oinv();
  bool oinv = inv0 ^ inv1;
  if ( inv0 ) {
    fanin0 = ~fanin0;
  }
  if ( inv1 ) {
    fanin1 = ~fanin1;
  }

  return new_node(true, oinv, fanin0, fanin1);
}

// @brief ハンドルの表す関数を返す．
ymuint16
NpnNodeMgr::handle_func(NpnHandle handle) const
{
  NpnNode* node = mNodeList[handle.node_id()];
  return xform_func(node->func(), handle.npn_xform());
}


BEGIN_NONAMESPACE

// 関数のXORサポートを求める．
ymuint
xorsup_vec(ymuint16 func)
{
  // 数が少ないので個別にやる．
  ymuint vec = 0U;

  // 0 番めの変数
  ymuint16 c0_0 = func & 0x5555U;
  ymuint16 c0_1 = (func & 0xaaaaU) >> 1;
  if ( (c0_0 ^ c0_1) == 0x5555U ) {
    vec |= 1U;
  }

  // 1 番めの変数
  ymuint16 c1_0 = func & 0x3333U;
  ymuint16 c1_1 = (func & 0xccccU) >> 2;
  if ( (c1_0 ^ c1_1) == 0x3333U ) {
    vec |= 2U;
  }

  // 2 番めの変数
  ymuint16 c2_0 = func & 0x0f0fU;
  ymuint16 c2_1 = (func & 0xf0f0U) >> 4;
  if ( (c2_0 ^ c2_1) == 0x0F0FU ) {
    vec |= 4U;
  }

  // 3 番めの変数
  ymuint16 c3_0 = func & 0x00ffU;
  ymuint16 c3_1 = (func & 0xff00U) >> 8;
  if ( (c3_0 ^ c3_1) == 0x00FFU ) {
    vec |= 8;
  }

  return vec;
}

// vect の中の最小要素を返す．
inline
ymuint
first_pos(ymuint vect)
{
  for (ymuint i = 0; i < 4; ++ i) {
    if ( vect & (1U << i) ){
      return i;
    }
  }
  return 0xFFFFU;
}

END_NONAMESPACE


// @brief 新しいノードを登録する関数
// @param[in] is_xor XOR ノードの時 true にするフラグ
// @param[in] oinv 出力の極性
// @param[in] fanin0, fanin1 ファンインのハンドル
// @note おなじノードが既に存在していたらそのノードを返す．
NpnHandle
NpnNodeMgr::new_node(bool is_xor,
		     bool oinv,
		     NpnHandle fanin0,
		     NpnHandle fanin1)
{
  ymint debug = 0;
  ymuint o_func = 0;

  if ( debug > 0 ) {
    cout << endl;
    cout << "new_node(";
    if ( is_xor ) {
      cout << "XOR";
    }
    else {
      cout << "AND";
    }
    cout << ")";
    if ( oinv ) {
      cout << " INV";
    }
    cout << endl;
    vector<NpnHandle> handle_list;
    handle_list.push_back(fanin0);
    handle_list.push_back(fanin1);
    dump_handle(cout, handle_list);

    if ( debug > 1 ) {
      cout << "func0  = ";
      print_func(cout, handle_func(fanin0));

      cout << "func1  = ";
      print_func(cout, handle_func(fanin1));

      cout << "result = ";
      o_func = calc_func(is_xor, fanin0, fanin1, oinv);
      print_func(cout, o_func);
    }
  }

  // Phase1: 準正規形にする．
  // 使われていない変数を削除して詰める．
  const Npn4Cannon& npn_normal = npn4norm[o_func];
  ymuint nfunc = npn_normal.mFunc;
  NpnXform cxf(npn_normal.mPerm);
  fanin0 = xform_handle(fanin0, cxf);
  fanin1 = xform_handle(fanin1, cxf);

  NpnXform post_oxf = inverse(cxf);
  if ( oinv ) {
    post_oxf.flip_oinv();
  }

  if ( debug > 1 ) {
    cout << endl;
    cout << "normalize" << endl;
    cout << "original func     = ";
    print_func(cout, o_func);
    cout << "normalizing xform = " << cxf << endl
	 << "normalized func   = ";
    print_func(cout, nfunc);
    cout << "fanin0            = " << fanin0 << endl
	 << "fanin1            = " << fanin1 << endl
	 << endl;

    ymuint new_func2 = calc_func(is_xor, fanin0, fanin1, post_oxf);
    if ( new_func2 != o_func ) {
      cout << "func0 = ";
      ymuint func0 = handle_func(fanin0);
      print_func(cout, func0);
      cout << "func1 = ";
      ymuint func1 = handle_func(fanin1);
      print_func(cout, func1);
      cout << "new func2 = ";
      print_func(cout, new_func2);
      cout << endl;
    }
    ASSERT_COND( new_func2 == o_func );
  }

  // Phase2: 左右の子供の順序を正規化する．
  NpnHandle new_fanin0;
  NpnHandle new_fanin1;
  NpnXform oxf;

  ymuint node0 = fanin0.node_id();
  ymuint node1 = fanin1.node_id();
  if ( node0 < node1 ) {
    normalize(fanin0, fanin1, new_fanin0, new_fanin1, oxf, is_xor);
    if ( debug > 1 ) {
      ymuint new_func = calc_func(is_xor, new_fanin0, new_fanin1, oxf * post_oxf);
      if ( new_func != o_func ) {
	cout << "new_fanin0 = " << new_fanin0 << endl
	     << "new_fanin1 = " << new_fanin1 << endl
	     << "new_func   = ";
	print_func(cout, new_func);
      }
      ASSERT_COND( new_func == o_func );
    }
  }
  else if ( node0 > node1 ) {
    normalize(fanin1, fanin0, new_fanin0, new_fanin1, oxf, is_xor);
    if ( debug > 1 ) {
      ymuint new_func = calc_func(is_xor, new_fanin0, new_fanin1, oxf * post_oxf);
      if ( new_func != o_func ) {
	cout << "new_fanin0 = " << new_fanin0 << endl
	     << "new_fanin1 = " << new_fanin1 << endl
	     << "new_func   = ";
	print_func(cout, new_func);
      }
      ASSERT_COND( new_func == o_func );
    }
  }
  else { // node0 == node1
    NpnHandle new1_fanin0;
    NpnHandle new1_fanin1;
    NpnXform new1_oxf;
    normalize(fanin0, fanin1, new1_fanin0, new1_fanin1, new1_oxf, is_xor);
    if ( debug > 1 ) {
      ymuint new_func = calc_func(is_xor, new1_fanin0, new1_fanin1, new1_oxf * post_oxf);
      if ( new_func != o_func ) {
	cout << "new_fanin0 = " << new1_fanin0 << endl
	     << "new_fanin1 = " << new1_fanin1 << endl
	     << "new_func   = ";
	print_func(cout, new_func);
      }
      ASSERT_COND( new_func == o_func );
    }

    NpnHandle new2_fanin0;
    NpnHandle new2_fanin1;
    NpnXform new2_oxf;
    normalize(fanin1, fanin0, new2_fanin0, new2_fanin1, new2_oxf, is_xor);
    if ( debug > 1 ) {
      ymuint new_func = calc_func(is_xor, new2_fanin0, new2_fanin1, new2_oxf * post_oxf);
      if ( new_func != o_func ) {
	cout << "new_fanin0 = " << new2_fanin0 << endl
	     << "new_fanin1 = " << new2_fanin1 << endl
	     << "new_func   = ";
	print_func(cout, new_func);
      }
      ASSERT_COND( new_func == o_func );
    }

    NpnXform a = new1_fanin1.npn_xform();
    NpnXform b = new2_fanin1.npn_xform();
    if ( a < b ) {
      new_fanin0 = new1_fanin0;
      new_fanin1 = new1_fanin1;
      oxf = new1_oxf;
    }
    else if ( a > b ) {
      new_fanin0 = new2_fanin0;
      new_fanin1 = new2_fanin1;
      oxf = new2_oxf;
    }
    else { // a == b
      // fanin0 の出力に反転属性があり，fanin1 の出力に反転属性がない場合
      // fanin0 と fanin1 を入れ替える．
      if ( new1_fanin0.oinv() && !new2_fanin0.oinv() ) {
	new_fanin0 = new2_fanin0;
	new_fanin1 = new2_fanin1;
	oxf = new2_oxf;
      }
      else {
	new_fanin0 = new1_fanin0;
	new_fanin1 = new1_fanin1;
	oxf = new1_oxf;
      }
    }
  }

  fanin0 = new_fanin0;
  fanin1 = new_fanin1;
  oxf *= post_oxf;

  ymuint type_pat = is_xor ? 3U : 2U;
  ymuint pos = unordered_func(fanin0, fanin1, is_xor);
  ymuint idx = pos % mHashSize;
  NpnNode* node = NULL;
  for (node = mHashTable[idx]; node; node = node->mSlink) {
    if ( node->mFanin[0] == fanin0 &&
	 node->mFanin[1] == fanin1 &&
	 (node->mId & 3U) == type_pat ) {
      break;
    }
  }
  if ( node == NULL ) {
    // 新しいノードを作る．
    NpnXform xf1 = fanin0.npn_xform();
    NpnXform xf2 = fanin1.npn_xform();
    NpnNode* node1 = this->node(fanin0.node_id());
    NpnNode* node2 = this->node(fanin1.node_id());
    ymuint sup_vec1 = node1->sup_vect() * xf1;
    ymuint sup_vec2 = node2->sup_vect() * xf2;

    ymuint node_func = calc_func(is_xor, fanin0, fanin1, false);
    node = alloc_node();
    node->mId |= type_pat;
    node->mFunc = node_func;
    node->mSupVect = sup_vec1 | sup_vec2;
    if ( is_xor ) {
      ymuint xsup_vec1 = node1->xorsup_vect() * xf1;
      ymuint xsup_vec2 = node2->xorsup_vect() * xf2;
      ymuint tmp1 = xsup_vec1 & ~sup_vec2;
      ymuint tmp2 = xsup_vec2 & ~sup_vec1;
      ymuint tmp3 = xsup_vec1 & xsup_vec2;
      node->mXorSupVect = tmp1 | tmp2 | tmp3;
    }
    else {
      node->mXorSupVect = 0U;
    }
    {
#if 0
      ymuint xsup_vec1 = node1->xorsup_vect() * xf1;
      ymuint xsup_vec2 = node2->xorsup_vect() * xf2;
      ymuint16 xorsup = xorsup_vec(node_func);
      if ( node->mXorSupVect != xorsup ) {
	cout << "sup_vec1          = " << hex << sup_vec1 << dec << endl
	     << "sup_vec2          = " << hex << sup_vec2 << dec << endl
	     << "xsup_vec1         = " << hex << xsup_vec1 << dec << endl
	     << "xsup_vec2         = " << hex << xsup_vec2 << dec << endl
	     << "node->mXorSupVect = " << hex << static_cast<ymuint>(node->mXorSupVect) << endl
	     << "xorsup            = " << hex << xorsup << endl;
      }
      ASSERT_COND( node->mXorSupVect == xorsup );
#endif
    }

    node->mFanin[0] = fanin0;
    node->mFanin[1] = fanin1;

    // ハッシュ表に登録する．
    if ( mAndList.size() >= mNextLimit ) {
      alloc_table(mHashSize * 2);
      // サイズが変わったのでインデックスを再計算する．
      idx = pos % mHashSize;
    }
    node->mSlink = mHashTable[idx];
    mHashTable[idx] = node;
    mAndList.push_back(node);

    // 同位体変換リストを作る．
    const vector<NpnXform>& ident_list1 = node1->ident_list();
    const vector<NpnXform>& ident_list2 = node2->ident_list();

    if ( debug > 2 ) {
      cout << "make ident list" << endl
	   << "  xf1      = " << xf1 << endl
	   << "  sup_vec1 = " << sup_vec1 << endl
	   << "  xf2      = " << xf2 << endl
	   << "  sup_vec2 = " << sup_vec2 << endl;
    }
    for (vector<NpnXform>::const_iterator p1 = ident_list1.begin();
	 p1 != ident_list1.end(); ++ p1) {
      NpnXform ixf1 = inverse(xf1) * (*p1) * xf1;
      if ( debug > 2 ) {
	cout << "    original ixf1 = " << *p1 << endl
	     << "    ixf1          = " << ixf1 << endl;
      }
      for (vector<NpnXform>::const_iterator p2 = ident_list2.begin();
	   p2 != ident_list2.end(); ++ p2) {
	NpnXform ixf2 = inverse(xf2) * (*p2) * xf2;
	if ( debug > 2 ) {
	  cout << "    original ixf2 = " << *p2 << endl
	       << "    ixf2          = " << ixf2 << endl;
	}
	NpnXform ixf;
	if ( merge(ixf1, sup_vec1, ixf2, sup_vec2, ixf) ) {
	  node->mIdentList.push_back(ixf);
	  if ( debug > 2 ) {
	    cout << ixf << endl;
	    // 構造に対する同位体変換だから必ず
	    // 関数に対しても同位体変換でなければならない．
	    ymuint tmp_func = xform_func(node_func, ixf);
	    ASSERT_COND( tmp_func == node_func );
	  }
	}
      }
    }
    if ( debug > 2 ) {
      cout << "make ident list: phase1 end" << endl;
    }

    if ( node1 == node2 && xf1.output_inv() == xf2.output_inv() ) {
      NpnXform ixf1 = inverse(xf1) * xf2;
      NpnXform ixf2 = inverse(xf2) * xf1;
      NpnXform ixf;
      if ( merge(ixf1, sup_vec1, ixf2, sup_vec2, ixf) ) {
	ymuint n = node->mIdentList.size();
	for (ymuint i = 0; i < n; ++ i) {
	  NpnXform xf = node->mIdentList[i] * ixf;
	  node->mIdentList.push_back(xf);
	  if ( debug > 2 ) {
	    cout << xf << endl;
	    // 構造に対する同位体変換だから必ず
	    // 関数に対しても同位体変換でなければならない．
	    ymuint tmp_func = xform_func(node_func, xf);
	    ASSERT_COND( tmp_func == node_func );
	  }
	}
      }
    }
    if ( debug > 2 ) {
      cout << "make ident list: phase2 end" << endl;
    }
  }

  if ( debug > 0 ) {
    NpnHandle handle(node->id(), oxf);
    ymuint func1 = handle_func(handle);
    if ( o_func != func1 ) {
      cout << "handle_func = ";
      print_func(cout, func1);
      cout << "o_func = ";
      print_func(cout, o_func);
    }
    ASSERT_COND( o_func == func1 );

    cout << "node->id() = " << node->id()
	 << ", oxf = " << oxf << endl;
    dump_handle(cout, NpnHandle(node->id(), oxf));
    cout << "ident_list =" << endl;
    for (vector<NpnXform>::iterator p = node->mIdentList.begin();
	 p != node->mIdentList.end(); ++ p) {
      cout << *p << endl;
    }
    cout << "=====<<new_node end>>======================================" << endl;
  }

  return NpnHandle(node->id(), oxf);
}

// @brief ファンインを正規化する．
void
NpnNodeMgr::normalize(NpnHandle fanin0,
		      NpnHandle fanin1,
		      NpnHandle& new_fanin0,
		      NpnHandle& new_fanin1,
		      NpnXform& oxf,
		      bool is_xor)
{
  bool debug = false;
  ymuint ofunc = 0;

  if ( debug ) {
    cout << "normalize" << endl
	 << " fanin0 = " << fanin0 << endl
	 << " fanin1 = " << fanin1 << endl;
    ofunc = calc_func(is_xor, fanin0, fanin1, NpnXform());
 }

  // fanin0 に対する同位体変換のリストを求める．
  NpnNode* node0 = node(fanin0.node_id());
  const vector<NpnXform>& ident0_list = node0->ident_list();
  // fanin1 に対する同位体変換のリストを求める．
  NpnNode* node1 = node(fanin1.node_id());
  const vector<NpnXform>& ident1_list = node1->ident_list();

  // 最終結果を保持しておく変数
  bool first = true;
  NpnHandle min_fanin0 = fanin0; // ダミー
  NpnHandle min_fanin1 = fanin1; // ダミー
  NpnXform min_oxf;

  // 個々の同位体変換を適用する．
  for (vector<NpnXform>::const_iterator q = ident0_list.begin();
       q != ident0_list.end(); ++ q) {
    NpnXform tmp_xf0 = fanin0.npn_xform();
    NpnXform xf = inverse(tmp_xf0) * (*q) * tmp_xf0;

    NpnHandle tmp_fanin0 = fanin0;

    NpnHandle tmp_fanin1 = xform_handle(fanin1, xf);

    if ( debug ) {
      ymuint tmp_func = calc_func(is_xor, tmp_fanin0, tmp_fanin1, inverse(xf));
      cout << "Phase-0:" << endl
	   << "xf         = " << xf << endl
	   << "tmp_fanin0 = " << tmp_fanin0 << endl
	   << "tmp_fanin1 = " << tmp_fanin1 << endl
	   << "tmp_func   = ";
      print_func(cout, tmp_func);
      cout << endl;
      if ( tmp_func != ofunc ) {
	abort();
      }
    }

    // Phase-1: fanin0 の変換が恒等変換になるように調節する．
    NpnXform xf0 = tmp_fanin0.npn_xform();
    if ( xf0.output_inv() ) {
      // 出力の反転属性は動かせないので外しておく．
      xf0.flip_oinv();
    }
    NpnXform ixf0 = inverse(xf0);
    tmp_fanin0 = xform_handle(tmp_fanin0, ixf0);
    tmp_fanin1 = xform_handle(tmp_fanin1, ixf0);
    NpnXform tmp_oxf = xf0 * inverse(xf);

    if ( debug ) {
      ymuint tmp_func = calc_func(is_xor, tmp_fanin0, tmp_fanin1, tmp_oxf);
      cout << "Phase-1:" << endl
	   << "tmp_oxf    = " << tmp_oxf << endl
	   << "tmp_fanin0 = " << tmp_fanin0 << endl
	   << "tmp_fanin1 = " << tmp_fanin1 << endl
	   << "tmp_func   = ";
      print_func(cout, tmp_func);
      cout << endl;
      if ( tmp_func != ofunc ) {
	abort();
      }
    }

    // Phase-2: tmp_fanin1 の同位体変換のうち最小のものを求める．
    for (vector<NpnXform>::const_iterator q1 = ident1_list.begin();
	 q1 != ident1_list.end(); ++ q1) {
      NpnXform tmp_xf1 = tmp_fanin1.npn_xform();
      NpnXform xf1 = inverse(tmp_xf1) * (*q1) * tmp_xf1;

      NpnHandle tmp2_fanin0 = tmp_fanin0;
      NpnHandle tmp2_fanin1 = xform_handle(tmp_fanin1, xf1);
      NpnXform tmp2_oxf = tmp_oxf;

      {
	if ( tmp2_fanin1.oinv() ) {
	  NpnXform tmp2_xf1 = tmp2_fanin1.npn_xform();
	  ymuint xorsup = node1->xorsup_vect();
	  if ( xorsup ) {
	    tmp2_xf1.flip_iinv(first_pos(xorsup));
	    tmp2_xf1.flip_oinv();
	    tmp2_fanin1.set(node1->id(), tmp2_xf1);
	  }
	}
      }

      if ( debug ) {
	ymuint tmp_func = calc_func(is_xor, tmp2_fanin0, tmp2_fanin1, tmp2_oxf);
	cout << "Phase-2:" << endl
	     << "xf1         = " << xf1 << endl
	     << "tmp2_oxf    = " << tmp2_oxf << endl
	     << "tmp2_fanin0 = " << tmp2_fanin0 << endl
	     << "tmp2_fanin1 = " << tmp2_fanin1 << endl
	     << "tmp_func    = ";
	print_func(cout, tmp_func);
	cout << endl;
	if ( tmp_func != ofunc ) {
	  abort();
	}
      }

      // Phase-3: tmp2_fanin1 の変数のうち，tmp_fanin0 に現れない変数
      //          の反転属性を削除する．
      NpnHandle tmp3_fanin0 = tmp2_fanin0;
      NpnHandle tmp3_fanin1 = tmp2_fanin1;
      NpnXform tmp3_oxf = tmp2_oxf;
      {
	NpnXform tmp_xf1 = tmp3_fanin1.npn_xform();
	NpnXform xfd;
	bool changed = false;
	ymuint sup0 = node0->sup_vect() * inverse(tmp_xf1);
	ymuint xorsup = node1->xorsup_vect();
	ymuint tmp_a = xorsup & ~sup0;
	NpnXform xfd1;
	for (ymuint i = 0; i < 4; ++ i) {
	  if ( tmp_xf1.input_inv(i) ) {
	    ymuint mask = (1U << i);
	    ymuint dst_i = tmp_xf1.input_perm(i);
	    if ( (sup0 & mask) == 0U ) {
	      // dst_i は fanin0 に現れない．
	      xfd.flip_iinv(dst_i);
	      xfd1.flip_iinv(dst_i);
	      changed = true;
	    }
	    else if ( (xorsup & mask) != 0U && tmp_a != 0U ) {
	      // dst_i で XOR 分解可能で
	      // ほかに XOR 分解可能でかつ fanin0 に現れない変数がある．
	      ymuint dst_j = tmp_xf1.input_perm(first_pos(tmp_a));
	      xfd.flip_iinv(dst_j);
	      xfd1.flip_iinv(dst_i);
	      changed = true;
	    }
	  }
	}
	if ( changed ) {
	  tmp3_fanin1 = xform_handle(tmp3_fanin1, xfd1);
	  tmp3_oxf = xfd * tmp3_oxf;
	}

	if ( debug ) {
	  ymuint tmp_func = calc_func(is_xor, tmp3_fanin0, tmp3_fanin1, tmp3_oxf);
	  cout << "Phase-3:" << endl
	       << "tmp3_oxf    = " << tmp3_oxf << endl
	       << "tmp3_fanin0 = " << tmp3_fanin0 << endl
	       << "tmp3_fanin1 = " << tmp3_fanin1 << endl
		 << "tmp_func  = ";
	  print_func(cout, tmp_func);
	  cout << endl;
	  if ( tmp_func != ofunc ) {
	    abort();
	  }
	}
      }

      // Phase-4: tmp4_fanin0 と tmp4_fanin1 の出力の反転属性の正規化を行なう．
      // 結果は tmp4_oxf に反映される．
      NpnHandle tmp4_fanin0 = tmp3_fanin0;
      NpnHandle tmp4_fanin1 = tmp3_fanin1;
      NpnXform tmp4_oxf = tmp3_oxf;
      {
	NpnXform tmp_xf0 = tmp4_fanin0.npn_xform();
	NpnXform tmp_xf1 = tmp4_fanin1.npn_xform();
	ymuint sup0 = node0->sup_vect();
	ymuint sup1 = node1->sup_vect() * tmp_xf1;
	ymuint xorsup0 = node0->xorsup_vect();
	ymuint xorsup1 = node1->xorsup_vect() * tmp_xf1;
	ymuint tmp_a = xorsup0 & ~sup1;
	ymuint tmp_b = xorsup1 & ~sup0;
	ymuint tmp_c = xorsup0 & xorsup1;
	if ( tmp_xf0.output_inv() ) {
	  if ( tmp_xf1.output_inv() ) {
	    // case 11: tmp4_fanin0 と tmp4_fanin1 の出力が両方反転している．
	    NpnXform tmp5_oxf = tmp4_oxf;
	    bool tmp5_valid = false;
	    if ( tmp_a != 0U && tmp_b != 0U ) {
	      tmp5_oxf.flip_iinv(first_pos(tmp_a));
	      tmp5_oxf.flip_iinv(first_pos(tmp_b));
	      tmp5_valid = true;
	    }
	    NpnXform tmp6_oxf = tmp4_oxf;
	    bool tmp6_valid = false;
	    if ( tmp_c ) {
	      tmp6_oxf.flip_iinv(first_pos(tmp_c));
	      tmp6_valid = true;
	    }
	    if ( tmp5_valid ) {
	      if ( tmp6_valid ) {
		if ( tmp5_oxf < tmp6_oxf ) {
		  tmp4_oxf = tmp5_oxf;
		}
		else {
		  tmp4_oxf = tmp6_oxf;
		}
	      }
	      else {
		tmp4_oxf = tmp5_oxf;
	      }
	    }
	    else if ( tmp6_valid ) {
	      tmp4_oxf = tmp6_oxf;
	    }
	    if ( tmp5_valid || tmp6_valid ) {
	      tmp4_fanin0.flip_oinv();
	      tmp4_fanin1.flip_oinv();
	    }
	    else if ( tmp_a ) {
	      tmp4_oxf.flip_iinv(first_pos(tmp_a));
	      tmp4_fanin0.flip_oinv();
	    }
	    else if ( tmp_b ) {
	      tmp4_oxf.flip_iinv(first_pos(tmp_b));
	      tmp4_fanin1.flip_oinv();
	    }
	  }
	  else {
	    // case 10: tmp4_fanin0 の出力が反転している．
	    if ( tmp_a ) {
	      tmp4_oxf.flip_iinv(first_pos(tmp_a));
	      tmp4_fanin0.flip_oinv();
	    }
	    else if ( tmp_c ) {
	      if ( tmp_b ) {
		tmp4_oxf.flip_iinv(first_pos(tmp_b));
		tmp4_oxf.flip_iinv(first_pos(tmp_c));
		tmp4_fanin0.flip_oinv();
	      }
	      else {
		tmp4_oxf.flip_iinv(first_pos(tmp_c));
		tmp4_fanin0.flip_oinv();
		tmp4_fanin1.flip_oinv();
	      }
	    }
	  }
	}
	else if ( tmp_xf1.output_inv() ) {
	  // case 01: tmp4_fanin1 の出力が反転している．
	  if ( tmp_b ) {
	    tmp4_oxf.flip_iinv(first_pos(tmp_b));
	    tmp4_fanin1.flip_oinv();
	  }
	  else if ( tmp_a != 0U && tmp_c != 0U ) {
	    tmp4_oxf.flip_iinv(first_pos(tmp_a));
	    tmp4_oxf.flip_iinv(first_pos(tmp_c));
	    tmp4_fanin1.flip_oinv();
	  }
	}
      }

      if ( debug ) {
	ymuint tmp_func = calc_func(is_xor, tmp4_fanin0, tmp4_fanin1, tmp4_oxf);
	cout << "Phase-4:" << endl
	     << "tmp4_oxf    = " << tmp4_oxf << endl
	     << "tmp4_fanin0 = " << tmp4_fanin0 << endl
	     << "tmp4_fanin1 = " << tmp4_fanin1 << endl
	     << endl
	     << "min_oxf    = " << min_oxf << endl
	     << "min_fanin0 = " << min_fanin0 << endl
	     << "min_fanin1 = " << min_fanin1 << endl
	     << "tmp_func = ";
	print_func(cout, tmp_func);
	cout << endl
	     << endl;
	if ( tmp_func != ofunc ) {
	  abort();
	}
      }

      if ( first ||
	   min_fanin0 > tmp4_fanin0 ||
	   min_fanin0 == min_fanin0 && min_fanin1 > tmp4_fanin1 ) {
	first = false;
	min_fanin0 = tmp4_fanin0;
	min_fanin1 = tmp4_fanin1;
	min_oxf = tmp4_oxf;
      }
    }
  }

  new_fanin0 = min_fanin0;
  new_fanin1 = min_fanin1;
  oxf = min_oxf;

  if ( debug ) {
    ymuint new_func = calc_func(is_xor, new_fanin0, new_fanin1, oxf);
    cout << "Final:" << endl
	 << "oxf        = " << oxf << endl
	 << "new_fanin0 = " << new_fanin0 << endl
	 << "new_fanin1 = " << new_fanin1 << endl
	 << "new_func   = ";
    print_func(cout, new_func);
    cout << endl;
  }
}

// 関数を求める．
ymuint
NpnNodeMgr::calc_func(bool is_xor,
		      NpnHandle fanin0,
		      NpnHandle fanin1,
		      NpnXform oxf)
{
  ymuint func0 = handle_func(fanin0);
  ymuint func1 = handle_func(fanin1);
  ymuint result_func = (is_xor) ? func0 ^ func1 : func0 & func1;
  return xform_func(result_func, oxf);
}

// @brief 関数を求める．
ymuint
NpnNodeMgr::calc_func(bool is_xor,
		      NpnHandle fanin0,
		      NpnHandle fanin1,
		      bool oinv)
{
  NpnXform oxf;
  if ( oinv ) {
    oxf.flip_oinv();
  }
  return calc_func(is_xor, fanin0, fanin1, oxf);
}

// @brief NpnHandle に NPN 変換を施す．
NpnHandle
NpnNodeMgr::xform_handle(NpnHandle handle,
			 NpnXform xf) const
{
  ymuint id = handle.node_id();
  NpnXform new_xf = handle.npn_xform();
  new_xf *= xf;
  NpnNode* node = this->node(id);
  ymuint sup_vec = node->sup_vect();
  new_xf.normalize(sup_vec);
  return NpnHandle(id, new_xf);
}

// @brief 展開したノード数を仮想的に返す．
ymuint
NpnNodeMgr::count(NpnHandle handle) const
{
  unordered_set<ymuint32> hash1;
  return count_sub(handle, hash1);
}

// @brief count() の下請け関数
ymuint
NpnNodeMgr::count_sub(NpnHandle handle,
		      unordered_set<ymuint32>& hash1) const
{
  NpnNode* node = mNodeList[handle.node_id()];
  if ( !node->is_logic() ) {
    return 0;
  }

  if ( handle.oinv() ) {
    handle.flip_oinv();
  }
#if 0
  if ( node->is_xor() ) {
    ymuint xorsup = node->xorsup_vect();
    for (ymuint i = 0; i < 4; ++ i) {
      if ( handle.iinv(i) && (xorsup & (1U << i)) ) {
	handle.flip_iinv(i);
      }
    }
  }
#endif
  ymuint sig = handle.hash();
  if ( hash1.count(sig) > 0 ) {
    return 0;
  }
  hash1.insert(sig);

  NpnXform xf = handle.npn_xform();
  ymuint ans = 1;
  ans += count_sub(xform_handle(node->fanin0(), xf), hash1);
  ans += count_sub(xform_handle(node->fanin1(), xf), hash1);
  return ans;
}


// @brief ノードを生成する関数
NpnNode*
NpnNodeMgr::alloc_node()
{
  void* p = mAlloc.get_memory(sizeof(NpnNode));
  ymuint id = mNodeList.size();
  NpnNode* node = new (p) NpnNode(id);
  mNodeList.push_back(node);
  return node;
}

// @brief ハンドルとその子供の内容を出力する．
void
NpnNodeMgr::dump_handle(ostream& s,
			NpnHandle handle) const
{
  vector<NpnHandle> tmp_list(1, handle);
  dump_handle(s, tmp_list);
}

// @brief 複数のハンドルとその子供の内容を出力する．
void
NpnNodeMgr::dump_handle(ostream& s,
			const vector<NpnHandle>& handle_list) const
{
  s << "----------------------------------------" << endl;
  ymuint i = 0;
  for (vector<NpnHandle>::const_iterator p = handle_list.begin();
       p != handle_list.end(); ++ p, ++ i) {
    NpnHandle handle = *p;
    s << "Handle#" << i << ": " << handle << endl;
  }
  s << "----------------------------------------" << endl;

  unordered_set<ymuint32> node_hash;
  for (vector<NpnHandle>::const_iterator p = handle_list.begin();
       p != handle_list.end(); ++ p) {
    NpnHandle handle = *p;
    dh_sub(s, handle.node_id(), node_hash);
  }
  s << "----------------------------------------" << endl;
}

// @brief dump_handle() の下請け関数
void
NpnNodeMgr::dh_sub(ostream& s,
		   ymuint id,
		   unordered_set<ymuint32>& node_hash) const
{
  if ( node_hash.count(id) > 0 ) {
    return;
  }
  node_hash.insert(id);

  NpnNode* node = mNodeList[id];

  if ( node->is_logic() ) {
    dh_sub(s, node->fanin0().node_id(), node_hash);
    dh_sub(s, node->fanin1().node_id(), node_hash);
  }

  s << "NODE#" << id << ": ";
  if ( node->is_const() ) {
    s << "CONST0" << endl;
  }
  else if ( node->is_input() ) {
    s << "INPUT" << endl;
  }
  else { // node->is_logic()
    if ( node->is_and() ) {
      s << "AND";
    }
    else {
      s << "XOR";
    }
    s << "( " << node->fanin0()
      << ", " << node->fanin1()
      << " )" << endl;
  }
}

// @brief ハンドルとその子供の内容を出力する．
void
NpnNodeMgr::dump_handle2(ostream& s,
			 NpnHandle handle) const
{
  vector<NpnHandle> tmp_list(1, handle);
  dump_handle2(s, tmp_list);
}

// @brief 複数のハンドルとその子供の内容を出力する．
void
NpnNodeMgr::dump_handle2(ostream& s,
			 const vector<NpnHandle>& handle_list) const
{
  s << "----------------------------------------" << endl;
  ymuint i = 0;
  for (vector<NpnHandle>::const_iterator p = handle_list.begin();
       p != handle_list.end(); ++ p, ++ i) {
    NpnHandle handle = *p;
    s << "Handle#" << i << ": " << handle << endl;
  }
  s << "----------------------------------------" << endl;

  unordered_set<ymuint32> node_hash;
  for (vector<NpnHandle>::const_iterator p = handle_list.begin();
       p != handle_list.end(); ++ p) {
    NpnHandle handle = *p;
    dh2_sub(s, handle, node_hash);
  }
  s << "----------------------------------------" << endl;
}

// @brief dump_handle2() の下請け関数
void
NpnNodeMgr::dh2_sub(ostream& s,
		    NpnHandle handle,
		    unordered_set<ymuint32>& node_hash) const
{
  ymuint id = handle.node_id();
  NpnNode* node = mNodeList[id];
  NpnXform xf = handle.npn_xform();
  NpnXform xf0 = xf.rep(node->sup_vect());
  NpnHandle rep_handle(id, xf0);
  ymuint sig = rep_handle.hash();
  if ( node_hash.count(sig) > 0 ) {
    return;
  }
  node_hash.insert(sig);

  if ( node->is_logic() ) {
    dh2_sub(s, xform_handle(node->fanin0(), xf0), node_hash);
    dh2_sub(s, xform_handle(node->fanin1(), xf0), node_hash);
  }

  s << rep_handle << ": ";
  if ( node->is_const() ) {
    if ( xf0.output_inv() ) {
      s << "CONST1" << endl;
    }
    else {
      s << "CONST0" << endl;
    }
  }
  else if ( node->is_input() ) {
    ymuint iid = xf0.input_perm(0);
    if ( xf0.input_inv(0) ) {
      s << "~";
    }
    s << "INPUT" << iid << endl;
  }
  else { // node->is_logic()
    if ( node->is_and() ) {
      s << "AND";
    }
    else {
      s << "XOR";
    }
    s << "( "
      << xform_handle(node->fanin0(), xf0)
      << " , "
      << xform_handle(node->fanin1(), xf0)
      << " )" << endl;
  }
}

// @brief ハッシュ関数
ymuint32
NpnNodeMgr::unordered_func(NpnHandle fanin0,
		      NpnHandle fanin1,
		      bool xor_flag)
{
  return fanin0.mData + fanin1.mData + xor_flag;
}

// @brief ハッシュテーブルを拡大する．
void
NpnNodeMgr::alloc_table(ymuint req_size)
{
  NpnNode** old_table = mHashTable;
  ymuint old_size = mHashSize;

  if ( mHashSize == 0 ) {
    mHashSize = 1024;
  }
  while ( mHashSize < req_size ) {
    mHashSize <<= 1;
  }

  mHashTable = new NpnNode*[mHashSize];
  for (ymuint i = 0; i < mHashSize; ++ i) {
    mHashTable[i] = NULL;
  }
  if ( old_size > 0 ) {
    for (ymuint i = 0; i < old_size; ++ i) {
      NpnNode* next = NULL;
      for (NpnNode* node = old_table[i]; node; node = next) {
	next = node->mSlink;
	ymuint pos = unordered_func(node->fanin0(), node->fanin1(), node->is_xor());
	ymuint idx = pos % mHashSize;
	node->mSlink = mHashTable[idx];
	mHashTable[idx] = node;
      }
    }
    delete [] old_table;
  }
  mNextLimit = static_cast<ymuint32>(mHashSize * 1.8);
}

END_NAMESPACE_YM
