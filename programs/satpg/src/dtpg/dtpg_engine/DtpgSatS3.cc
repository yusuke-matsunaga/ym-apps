﻿
/// @file DtpgSatS3.cc
/// @brief DtpgSatS3 の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2010, 2012-2014 Yusuke Matsunaga
/// All rights reserved.


#include "DtpgSatS3.h"

#include "DtpgStats.h"
#include "TpgNetwork.h"
#include "TpgNode.h"
#include "TpgFault.h"
#include "NodeSet.h"
#include "SatEngine.h"
#include "GenVidMap.h"
#include "ModelValMap.h"
#include "Extractor.h"
#include "NodeValList.h"
#include "YmUtils/HashSet.h"


BEGIN_NAMESPACE_YM_SATPG

BEGIN_NONAMESPACE
const bool do_verify = true;
END_NONAMESPACE

// @brief Single エンジンを作る．
// @param[in] sat_type SATソルバの種類を表す文字列
// @param[in] sat_option SATソルバに渡すオプション文字列
// @param[in] sat_outp SATソルバ用の出力ストリーム
// @param[in] bt バックトレーサー
// @param[in] dop パタンが求められた時に実行されるファンクタ
// @param[in] uop 検出不能と判定された時に実行されるファンクタ
DtpgEngine*
new_DtpgSatS3(const string& sat_type,
	      const string& sat_option,
	      ostream* sat_outp,
	      BackTracer& bt,
	      DetectOp& dop,
	      UntestOp& uop)
{
  return new DtpgSatS3(sat_type, sat_option, sat_outp, bt, dop, uop);
}

// @brief コンストラクタ
DtpgSatS3::DtpgSatS3(const string& sat_type,
		     const string& sat_option,
		     ostream* sat_outp,
		     BackTracer& bt,
		     DetectOp& dop,
		     UntestOp& uop) :
  DtpgSat(sat_type, sat_option, sat_outp, bt, dop, uop)
{
}

// @brief デストラクタ
DtpgSatS3::~DtpgSatS3()
{
}

BEGIN_NONAMESPACE

void
add_assumption(SatEngine& engine,
	       const NodeValList& as_list,
	       const VidMap& gvar_map)
{
  ymuint n = as_list.size();
  for (ymuint i = 0; i < n; ++ i) {
    NodeVal nv = as_list[i];
    TpgNode* node = nv.node();
    bool val = nv.val();
    Literal alit(gvar_map(node), !val);
    engine.assumption_add(alit);
  }
}

void
dfs(TpgNode* node,
    HashSet<ymuint>& mark,
    vector<TpgNode*>& node_list)
{
  if ( mark.check(node->id()) ) {
    return;
  }
  mark.add(node->id());

  node_list.push_back(node);

  ymuint nfo = node->active_fanout_num();
  for (ymuint i = 0; i < nfo; ++ i) {
    TpgNode* onode = node->active_fanout(i);
    dfs(onode, mark, node_list);
  }
}

struct NodeLt
{
  bool
  operator()(TpgNode* left,
	     TpgNode* right)
  {
    return left->id() < right->id();
  }
};

END_NONAMESPACE

// @brief テスト生成を行なう．
// @param[in] network 対象のネットワーク
// @param[in] stats 結果を格納する構造体
void
DtpgSatS3::run(TpgNetwork& network,
	       DtpgStats& stats)
{
  clear_stats();

  ymuint nn = network.active_node_num();
  ymuint max_fault_id = 0;
  for (ymuint i = 0; i < nn; ++ i) {
    TpgNode* node = network.active_node(i);
    if ( node->is_output() ) {
      continue;
    }

    ymuint nf = node->fault_num();
    for (ymuint i = 0; i < nf; ++ i) {
      TpgFault* f = node->fault(i);
      if ( f->status() != kFsDetected &&
	   !f->is_skip() ) {
	ymuint fid = f->id();
	if ( max_fault_id < fid ) {
	  max_fault_id = fid;
	}
      }
    }
  }

  ++ max_fault_id;
  mFaultInfoArray.clear();
  mFaultInfoArray.resize(max_fault_id);

  NodeSet node_set;

  // 一回目は普通のテスト生成と必要割当の検査
  vector<TpgFault*> fault_list;
  ymuint max_id = network.max_node_id();
  for (ymuint i = 0; i < nn; ++ i) {
    TpgNode* node = network.active_node(i);
    if ( node->is_output() ) {
      continue;
    }

    node_set.mark_region(max_id, vector<TpgNode*>(1, node));

    ymuint nf = node->fault_num();
    for (ymuint i = 0; i < nf; ++ i) {
      TpgFault* f = node->fault(i);
      FaultInfo& fi = mFaultInfoArray[f->id()];
      fi.mDetected = run_single(node_set, f);
      if ( fi.mDetected ) {
	fault_list.push_back(f);
      }
    }
  }

  for (ymuint i = 0; i < nn; ++ i) {
    TpgNode* node = network.active_node(i);
    if ( node->is_output() ) {
      continue;
    }

    node_set.mark_region(max_id, vector<TpgNode*>(1, node));

    ymuint nf = node->fault_num();
    for (ymuint i = 0; i < nf; ++ i) {
      TpgFault* f = node->fault(i);
      FaultInfo& fi = mFaultInfoArray[f->id()];
      if ( fi.mDetected ) {
	check_other_faults(network, node_set, f, fault_list);
      }
    }
  }

  ymuint nf = 0;
  for (ymuint i = 0; i < fault_list.size(); ++ i) {
    TpgFault* f = fault_list[i];
    FaultInfo& fi = mFaultInfoArray[f->id()];
    if ( fi.mDominaterList.empty() ) {
      ++ nf;
    }
    cout << f->str() << ":";
    const NodeValList& suf_list = fi.mSufList;
    for (ymuint j = 0; j < suf_list.size(); ++ j) {
      NodeVal nv = suf_list[j];
      cout << " " << nv.node()->id();
      if ( nv.val() ) {
	cout << "P";
      }
      else {
	cout << "N";
      }
    }
    cout << endl;
    const NodeValList& ma_list = fi.mMaList;
    cout << "    ";
    for (ymuint j = 0; j < ma_list.size(); ++ j) {
      NodeVal nv = ma_list[j];
      cout << " " << nv.node()->id();
      if ( nv.val() ) {
	cout << "P";
      }
      else {
	cout << "N";
      }
    }
    cout << endl;
  }
  cout << "Total " << fault_list.size() << " detectable faults" << endl
       << "Total " << nf << " representative faults" << endl;

  print_network(cout, network);

#if 0
  // 二回目は正常回路の CNF のみを作る
  {
    SatEngine engine(sat_type(), sat_option(), sat_outp());

    GenVidMap gvar_map(network.max_node_id());

    //////////////////////////////////////////////////////////////////////
    // 変数の割当
    //////////////////////////////////////////////////////////////////////
    for (ymuint i = 0; i < nn; ++ i) {
      TpgNode* node = network.active_node(i);
      VarId gvar = engine.new_var();
      gvar_map.set_vid(node, gvar);
    }

    //////////////////////////////////////////////////////////////////////
    // 正常回路の CNF を生成
    //////////////////////////////////////////////////////////////////////
    for (ymuint i = 0; i < nn; ++ i) {
      TpgNode* node = network.active_node(i);
      engine.make_node_cnf(node, gvar_map);
    }

    ymuint ng = 0;

    while ( !fault_list.empty() ) {
      // fault_list 中で必要割当が最大のものを探す．
      ymuint max = 0;
      TpgFault* max_f = NULL;
      ymuint max_pos = 0;
      ymuint nf = fault_list.size();
      for (ymuint i = 0; i < nf; ++ i) {
	TpgFault* f = fault_list[i];
	FaultInfo& fi = mFaultInfoArray[f->id()];
	ymuint n = fi.mSufList.size();
	if ( max < n ) {
	  max = n;
	  max_f = f;
	  max_pos = i;
	}
      }

      cout << "[" << ng << "]: Select " << max_f->str();
      cout.flush();

      ++ ng;

      // max_f を核にして同時に検出できる故障をマージしてゆく．
      NodeValList cur_assign;
      FaultInfo& fi = mFaultInfoArray[max_f->id()];
      cur_assign = fi.mSufList;

      vector<bool> mark(nf, false);
      vector<bool> conflict(nf, false);
      mark[max_pos] = true;
      for ( ; ; ) {
	ymuint min_assign = 0;
	ymuint min_fpos = 0;
	bool found = false;
	for (ymuint i = 0; i < nf; ++ i) {
	  if ( mark[i] || conflict[i] ) {
	    continue;
	  }

	  TpgFault* f = fault_list[i];
	  FaultInfo& fi = mFaultInfoArray[f->id()];

	  // cur_assign と fi.mSufList がコンフリクトしていないか調べる．
	  if ( check_conflict(cur_assign, fi.mSufList) ) {
	    conflict[i] = true;
	    continue;
	  }

	  NodeValList tmp_list = fi.mSufList;
	  tmp_list.merge(cur_assign);

	  ymuint new_assign = tmp_list.size() - cur_assign.size();
	  if ( !found || min_assign > new_assign ) {
	    found = true;
	    min_assign = new_assign;
	    min_fpos = i;
	  }
	}
	if ( !found ) {
	  break;
	}

	TpgFault* min_f = fault_list[min_fpos];
	FaultInfo& fi = mFaultInfoArray[min_f->id()];

	// cur_assign + fi.mSufList が充足可能か調べる．

	engine.assumption_begin();
	add_assumption(engine, cur_assign, gvar_map);
	add_assumption(engine, fi.mSufList, gvar_map);

	vector<Bool3> tmp_model;
	Bool3 stat = engine.solve(tmp_model);
	if ( stat == kB3True ) {
	  // 充足可能だった．
	  cur_assign.merge(fi.mSufList);
	  mark[min_fpos] = true;
	  cout << ".";
	  cout.flush();
	}
	else {
	  // 充足不能だった．
	  conflict[min_fpos] = true;
	}
      }
      cout << endl;

      // fault_list の更新
      ymuint wpos = 0;
      for (ymuint i = 0; i < nf; ++ i) {
	if ( !mark[i] ) {
	  if ( wpos < i ) {
	    fault_list[wpos] = fault_list[i];
	  }
	  ++ wpos;
	}
      }
      if ( wpos < nf ) {
	fault_list.erase(fault_list.begin() + wpos, fault_list.end());
      }
    }

    cout << "Total " << ng << " patterns" << endl;
  }
#endif

  get_stats(stats);
}

// @brief テストパタン生成を行なう．
// @param[in] node_set ノード集合
// @param[in] flist 故障リスト
bool
DtpgSatS3::run_single(const NodeSet& node_set,
		      TpgFault* fault)
{
  TpgNode* fnode = fault->node();

  cnf_begin();

  SatEngine engine(sat_type(), sat_option(), sat_outp());

  ymuint max_id = node_set.max_id();

  GenVidMap gvar_map(max_id);
  GenVidMap fvar_map(max_id);
  GenVidMap dvar_map(max_id);

  //////////////////////////////////////////////////////////////////////
  // 変数の割当
  //////////////////////////////////////////////////////////////////////
  for (ymuint i = 0; i < node_set.tfo_tfi_size(); ++ i) {
    TpgNode* node = node_set.tfo_tfi_node(i);
    VarId gvar = engine.new_var();
    gvar_map.set_vid(node, gvar);
    fvar_map.set_vid(node, gvar);
  }
  for (ymuint i = 0; i < node_set.tfo_size(); ++ i) {
    TpgNode* node = node_set.tfo_tfi_node(i);
    VarId fvar = engine.new_var();
    VarId dvar = engine.new_var();
    fvar_map.set_vid(node, fvar);
    dvar_map.set_vid(node, dvar);
  }

  //////////////////////////////////////////////////////////////////////
  // 正常回路の CNF を生成
  //////////////////////////////////////////////////////////////////////
  for (ymuint i = 0; i < node_set.tfo_tfi_size(); ++ i) {
    TpgNode* node = node_set.tfo_tfi_node(i);
    engine.make_node_cnf(node, gvar_map);
  }

  //////////////////////////////////////////////////////////////////////
  // 故障回路の CNF を生成
  //////////////////////////////////////////////////////////////////////
  for (ymuint i = 0; i < node_set.tfo_size(); ++ i) {
    TpgNode* node = node_set.tfo_tfi_node(i);

    // 故障回路のゲートの入出力関係を表すCNFを作る．
    if ( node == fnode ) {
      engine.make_fault_cnf(fault, gvar_map, fvar_map);
    }
    else {
      engine.make_node_cnf(node, fvar_map);
    }

    // D-Chain 制約を作る．
    engine.make_dchain_cnf(node, gvar_map, fvar_map, dvar_map);
  }

  //////////////////////////////////////////////////////////////////////
  // 故障の検出条件
  //////////////////////////////////////////////////////////////////////
  ymuint npo = node_set.output_list().size();
  engine.tmp_lits_begin(npo);
  for (ymuint i = 0; i < npo; ++ i) {
    TpgNode* node = node_set.output_list()[i];
    Literal dlit(dvar_map(node), false);
    engine.tmp_lits_add(dlit);
  }
  engine.tmp_lits_end();

  cnf_end();

  // 故障に対するテスト生成を行なう．
  engine.assumption_begin();

  // dominator ノードの dvar は1でなければならない．
  for (TpgNode* node = fnode; node != NULL; node = node->imm_dom()) {
    Literal dlit(dvar_map(node), false);
    engine.assumption_add(dlit);
  }

  vector<Bool3> sat_model;
  Bool3 sat_ans = engine.solve(sat_model);
  if ( sat_ans != kB3True ) {
    return false;
  }

  // 十分割当を求める．
  ModelValMap val_map(gvar_map, fvar_map, sat_model);
  Extractor extract(val_map);
  NodeValList& suf_list = mFaultInfoArray[fault->id()].mSufList;
  extract(fault, suf_list);

  if ( do_verify ) { // 検証
    SatEngine engine(sat_type(), sat_option(), sat_outp());

    GenVidMap gvar_map(max_id);
    GenVidMap fvar_map(max_id);
    GenVidMap dvar_map(max_id);

    //////////////////////////////////////////////////////////////////////
    // 変数の割当
    //////////////////////////////////////////////////////////////////////
    for (ymuint i = 0; i < node_set.tfo_tfi_size(); ++ i) {
      TpgNode* node = node_set.tfo_tfi_node(i);
      VarId gvar = engine.new_var();
      gvar_map.set_vid(node, gvar);
      fvar_map.set_vid(node, gvar);
    }
    for (ymuint i = 0; i < node_set.tfo_size(); ++ i) {
      TpgNode* node = node_set.tfo_tfi_node(i);
      VarId fvar = engine.new_var();
      VarId dvar = engine.new_var();
      fvar_map.set_vid(node, fvar);
      dvar_map.set_vid(node, dvar);
    }

    //////////////////////////////////////////////////////////////////////
    // 正常回路の CNF を生成
    //////////////////////////////////////////////////////////////////////
    for (ymuint i = 0; i < node_set.tfo_tfi_size(); ++ i) {
      TpgNode* node = node_set.tfo_tfi_node(i);
      engine.make_node_cnf(node, gvar_map);
    }

    //////////////////////////////////////////////////////////////////////
    // 故障回路の CNF を生成
    //////////////////////////////////////////////////////////////////////
    for (ymuint i = 0; i < node_set.tfo_size(); ++ i) {
      TpgNode* node = node_set.tfo_tfi_node(i);

      // 故障回路のゲートの入出力関係を表すCNFを作る．
      if ( node == fnode ) {
	engine.make_fault_cnf(fault, gvar_map, fvar_map);
      }
      else {
	engine.make_node_cnf(node, fvar_map);
      }

      // D-Chain 制約を作る．
      engine.make_dchain_cnf(node, gvar_map, fvar_map, dvar_map);
    }

    // 上の割当のもとでは常に故障が検出できることを検証する．
    engine.assumption_begin();

    ymuint n = suf_list.size();
    for (ymuint i = 0; i < n; ++ i) {
      NodeVal nv = suf_list[i];
      TpgNode* node = nv.node();
      VarId vid = gvar_map(node);
      bool inv = nv.val();
      Literal alit(vid, !inv);
      engine.assumption_add(alit);
    }

    //////////////////////////////////////////////////////////////////////
    // 故障の非検出条件
    //////////////////////////////////////////////////////////////////////
    ymuint npo = node_set.output_list().size();
    for (ymuint i = 0; i < npo; ++ i) {
      TpgNode* node = node_set.output_list()[i];
      Literal dlit(dvar_map(node), false);
      engine.assumption_add(~dlit);
    }
    vector<Bool3> tmp_model;
    Bool3 tmp_stat = engine.solve(tmp_model);
    if ( tmp_stat == kB3True ) {
      cout << fault->str() << endl;
      cout << "ERROR: not a sufficient condition" << endl;
      {
	ModelValMap tmp_val(gvar_map, fvar_map, tmp_model);
	ModelValMap sat_val(gvar_map, fvar_map, sat_model);
	HashSet<ymuint> mark;
	vector<TpgNode*> node_list;
	dfs(fault->node(), mark, node_list);
	sort(node_list.begin(), node_list.end(), NodeLt());
	for (ymuint i = 0; i < node_list.size(); ++ i) {
	  TpgNode* node = node_list[i];

	  cout << "Node#" << setw(5) << node->id() << ": ";
	  if ( tmp_val.gval(node) != sat_val.gval(node) ||
	       tmp_val.fval(node) != sat_val.fval(node) ) {
	    cout << "*";
	  }
	  else {
	    cout << " ";
	  }
	  cout << tmp_val.gval(node)
	       << "/" << tmp_val.fval(node)
	       << " <=> " << sat_val.gval(node)
	       << "/" << sat_val.fval(node)
	       << ": ";
	  if ( node->is_input() ) {
	    cout << "INPUT#" << node->input_id();
	  }
	  else if ( node->is_output() ) {
	    cout << "OUTPUT#" << node->output_id();
	    const TpgNode* inode = node->fanin(0);
	    cout << " = Node#" << inode->id();
	  }
	  else if ( node->is_logic() ) {
	    cout << node->gate_type();
	    ymuint ni = node->fanin_num();
	    if ( ni > 0 ) {
	      cout << "(";
	      for (ymuint j = 0; j < ni; ++ j) {
		const TpgNode* inode = node->fanin(j);
		cout << " Node#" << inode->id();
	      }
	      cout << ")";
	    }
	  }
	  else {
	    ASSERT_NOT_REACHED;
	  }
	  cout << endl;
	}

	for (ymuint i = 0; i < n; ++ i) {
	  NodeVal nv = suf_list[i];
	  TpgNode* node = nv.node();
	  if ( node_set.tfo_mark(node) ) {
	    continue;
	  }
	  cout << "Node#" << node->id() << ": ";
	  cout << ", gval = " << tmp_val.gval(node)
	       << ": ";
	  if ( node->is_input() ) {
	    cout << "INPUT#" << node->input_id();
	  }
	  else if ( node->is_output() ) {
	    cout << "OUTPUT#" << node->output_id();
	    const TpgNode* inode = node->fanin(0);
	    cout << " = Node#" << inode->id();
	  }
	  else if ( node->is_logic() ) {
	    cout << node->gate_type();
	    ymuint ni = node->fanin_num();
	    if ( ni > 0 ) {
	      cout << "(";
	      for (ymuint j = 0; j < ni; ++ j) {
		const TpgNode* inode = node->fanin(j);
		cout << " Node#" << inode->id();
	      }
	      cout << ")";
	    }
	  }
	  else {
	    ASSERT_NOT_REACHED;
	  }
	  cout << endl;
	}
	for (ymuint i = 0; i < n; ++ i) {
	  NodeVal nv = suf_list[i];
	  TpgNode* node = nv.node();
	  cout << " Node#" << node->id() << ":";
	  if ( nv.val() ) {
	    cout << "1";
	  }
	  else {
	    cout << "0";
	  }
	}
	cout << endl;
	abort();
      }
    }
  }

  // 必要割当を求める．
  vector<Bool3> tmp_model;
  NodeValList& ma_list = mFaultInfoArray[fault->id()].mMaList;
#if 1
  ymuint n = suf_list.size();
  for (ymuint i = 0; i < n; ++ i) {
    NodeVal nv = suf_list[i];

    engine.assumption_begin();

    // dominator ノードの dvar は1でなければならない．
    for (TpgNode* node = fnode; node != NULL; node = node->imm_dom()) {
      Literal dlit(dvar_map(node), false);
      engine.assumption_add(dlit);
    }

    // node の割当の反対を試す．
    TpgNode* node = nv.node();
    bool inv = nv.val();
    Literal alit(gvar_map(node), inv);
    engine.assumption_add(alit);

    Bool3 tmp_stat = engine.solve(tmp_model);
    if ( tmp_stat == kB3True ) {
      // 反対でも検出できたので必要割当ではない．
      ;
    }
    else if ( tmp_stat == kB3False ) {
      // 失敗したということはこの割当は必要である．
      ma_list.add(node, nv.val());
    }
    else {
      // アボート．とりあえず無視
      ;
    }
  }
#else
  for (ymuint i = 0; i < tfo_tfi_size(); ++ i) {
    TpgNode* node = tfo_tfi_node(i);
    Bool3 nval = sat_model[node->gvar().val()];
    engine.assumption_begin();

    // dominator ノードの dvar は1でなければならない．
    for (TpgNode* node = fnode; node != NULL; node = node->imm_dom()) {
      Literal dlit(dvar_map(node), false);
      engine.assumption_add(dlit);
    }

    // node の割当の反対を試す．
    bool inv = (nval == kB3True);
    Literal alit(gvar_map(node), inv);
    engine.assumption_add(alit);

    Bool3 tmp_stat = engine.solve(tmp_model);
    if ( tmp_stat == kB3True ) {
      // 反対でも検出できたので必要割当ではない．
      ;
    }
    else if ( tmp_stat == kB3False ) {
      // 失敗したということはこの割当は必要である．
      ma_list.add(node, nval);
    }
    else {
      // アボート．とりあえず無視
      ;
    }
  }
  suf_list.merge(ma_list);
#endif

  cout << fault->str() << ": sufficient assignment = " << suf_list.size()
       << ", mandatory assignment = " << ma_list.size() << endl;

  return true;
}

// @brief 他の故障との関係を調べる．
void
DtpgSatS3::check_other_faults(TpgNetwork& network,
			      const NodeSet& node_set,
			      TpgFault* f1,
			      const vector<TpgFault*>& fault_list)
{
  TpgNode* fnode = f1->node();

  cnf_begin();

  SatEngine engine(sat_type(), sat_option(), sat_outp());

  ymuint max_id = network.max_node_id();

  GenVidMap gvar_map(max_id);
  GenVidMap fvar_map(max_id);
  GenVidMap dvar_map(max_id);

  //////////////////////////////////////////////////////////////////////
  // 変数の割当
  //////////////////////////////////////////////////////////////////////
  for (ymuint i = 0; i < network.active_node_num(); ++ i) {
    TpgNode* node = network.active_node(i);
    VarId gvar = engine.new_var();
    gvar_map.set_vid(node, gvar);
    fvar_map.set_vid(node, gvar);
  }
  for (ymuint i = 0; i < node_set.tfo_size(); ++ i) {
    TpgNode* node = node_set.tfo_tfi_node(i);
    VarId fvar = engine.new_var();
    VarId dvar = engine.new_var();
    fvar_map.set_vid(node, fvar);
    dvar_map.set_vid(node, dvar);
  }

  //////////////////////////////////////////////////////////////////////
  // 正常回路の CNF を生成
  //////////////////////////////////////////////////////////////////////
  for (ymuint i = 0; i < network.active_node_num(); ++ i) {
    TpgNode* node = network.active_node(i);
    engine.make_node_cnf(node, gvar_map);
  }

  //////////////////////////////////////////////////////////////////////
  // 故障回路の CNF を生成
  //////////////////////////////////////////////////////////////////////
  for (ymuint i = 0; i < node_set.tfo_size(); ++ i) {
    TpgNode* node = node_set.tfo_tfi_node(i);

    // 故障回路のゲートの入出力関係を表すCNFを作る．
    if ( node == fnode ) {
      engine.make_fault_cnf(f1, gvar_map, fvar_map);
    }
    else {
      engine.make_node_cnf(node, fvar_map);
    }

    // D-Chain 制約を作る．
    engine.make_dchain_cnf(node, gvar_map, fvar_map, dvar_map);
  }

  for (vector<TpgFault*>::const_iterator p = fault_list.begin();
       p != fault_list.end(); ++ p) {
    TpgFault* f2 = *p;
    if ( f1 == f2 ) {
      continue;
    }

    // f2 の十分割当のものとで f1 が検出不能かどうか調べる．
    FaultInfo& fi2 = mFaultInfoArray[f2->id()];
    //const NodeValList& suf_list2 = fi2.mSufList;
    const NodeValList& suf_list2 = fi2.mMaList;

    // 故障に対するテスト生成を行なう．
    engine.assumption_begin();

    {
      ymuint npo = node_set.output_list().size();
      for (ymuint i = 0; i < npo; ++ i) {
	TpgNode* node = node_set.output_list()[i];
	Literal dlit(dvar_map(node), false);
	engine.assumption_add(~dlit);
      }
    }

    for (ymuint i = 0; i < suf_list2.size(); ++ i) {
      NodeVal nv = suf_list2[i];
      TpgNode* node1 = nv.node();
      Literal mlit(gvar_map(node1), false);
      if ( nv.val() ) {
	engine.assumption_add(mlit);
      }
      else {
	engine.assumption_add(~mlit);
      }
    }
    vector<Bool3> tmp_model;
    Bool3 tmp_stat = engine.solve(tmp_model);
    if ( tmp_stat == kB3True ) {
      // 検出できなかった．
      ;
    }
    else if ( tmp_stat == kB3False ) {
      // 検出できた．
      // ということは f2 を検出する時に常に f1 も検出できることを意味する．
      mFaultInfoArray[f2->id()].mDominateeList.push_back(f1->id());
      mFaultInfoArray[f1->id()].mDominaterList.push_back(f2->id());
      cout << f1->str() << " is dominated by " << f2->str() << endl;
      ;
    }
    else {
      // アボート．とりあえず無視
      ;
    }
  }

#if 0
  //////////////////////////////////////////////////////////////////////
  // 故障の検出条件
  //////////////////////////////////////////////////////////////////////
  ymuint npo = output_list().size();
  engine.tmp_lits_begin(npo);
  for (ymuint i = 0; i < npo; ++ i) {
    TpgNode* node = output_list()[i];
    Literal dlit(node->dvar(), false);
    engine.tmp_lits_add(dlit);
  }
  engine.tmp_lits_end();

  cnf_end();

  cout << f1->str() << ":";
  for (vector<TpgFault*>::const_iterator p = fault_list.begin();
       p != fault_list.end(); ++ p) {
    TpgFault* f2 = *p;

    // f2 の必要割当のもとで f1 が求められるか調べる．
    FaultInfo& fi2 = mFaultInfoArray[f2->id()];
    const NodeValList& suf_list2 = fi2.mSufList;

    // 故障に対するテスト生成を行なう．
    engine.assumption_begin();

    // dominator ノードの dvar は1でなければならない．
    for (TpgNode* node = fnode; node != NULL; node = node->imm_dom()) {
      Literal dlit(node->dvar(), false);
      engine.assumption_add(dlit);
    }

    for (ymuint i = 0; i < suf_list2.size(); ++ i) {
      NodeVal nv = suf_list2[i];
      TpgNode* node1 = nv.node();
      Literal mlit(node1->gvar(), false);
      if ( nv.val() ) {
	engine.assumption_add(mlit);
      }
      else {
	engine.assumption_add(~mlit);
      }
    }

    vector<Bool3> tmp_model;
    Bool3 tmp_stat = engine.solve(tmp_model);
    if ( tmp_stat == kB3True ) {
      // 検出できた．
      ;
    }
    else if ( tmp_stat == kB3False ) {
      // 検出できなかった．
      mFaultInfoArray[f1->id()].mConflictList.push_back(f2->id());
      cout << " " << f2->str();
    }
    else {
      // アボート．とりあえず無視
      ;
    }
  }
  cout << endl;
#endif
}

// @brief コンストラクタ
DtpgSatS3::FaultInfo::FaultInfo()
{
  mDetected = false;
}

// @brief デストラクタ
DtpgSatS3::FaultInfo::~FaultInfo()
{
}

END_NAMESPACE_YM_SATPG
