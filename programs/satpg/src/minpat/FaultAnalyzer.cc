﻿
/// @file FaultAnalyzer.cc
/// @brief FaultAnalyzer の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2011, 2013-2014, 2015 Yusuke Matsunaga
/// All rights reserved.


#include "FaultAnalyzer.h"

#include "ModelValMap.h"
#include "Extractor.h"
#include "BackTracer.h"

#include "TpgNetwork.h"
#include "TpgNode.h"
#include "TpgFault.h"
#include "TvMgr.h"
#include "TestVector.h"
#include "Fsim.h"
#include "KDet2Op.h"
#include "NodeSet.h"
#include "NodeValList.h"

#include "GvalCnf.h"
#include "FvalCnf.h"
#include "SatEngine.h"

#include "TpgCnf1.h"

#include "YmUtils/StopWatch.h"
#include "YmUtils/HashSet.h"


BEGIN_NAMESPACE_YM_SATPG

BEGIN_NONAMESPACE

struct FaultLt
{
  FaultLt(const vector<FaultInfo>& fault_info_array) :
    mFaultInfoArray(fault_info_array)
  {
  }

  bool
  operator()(ymuint left,
	     ymuint right)
  {
    return mFaultInfoArray[left].mDomCandList.size() < mFaultInfoArray[right].mDomCandList.size();
  }

  const vector<FaultInfo>& mFaultInfoArray;

};

// 2つのリストが共通要素を持つとき true を返す．
// リストはソートされていると仮定する．
inline
bool
check_intersect(const vector<ymuint>& list1,
		const vector<ymuint>& list2)
{
  ymuint n1 = list1.size();
  ymuint n2 = list2.size();
  ymuint i1 = 0;
  ymuint i2 = 0;
  ymuint v1 = list1[i1];
  ymuint v2 = list2[i2];
  for ( ; ; ) {
    if ( v1 < v2 ) {
      ++ i1;
      if ( i1 >= n1 ) {
	return false;
      }
      v1 = list1[i1];
    }
    else if ( v1 > v2 ) {
      ++ i2;
      if ( i2 >= n2 ) {
	return false;
      }
      v2 = list2[i2];
    }
    else {
      return true;
    }
  }
  return false;
}

// パタン番号リストの包含関係を調べる．
// 0 bit: list1 のみの要素がある．
// 1 bit: list2 のみの要素がある．
//
// リストの内容は昇順にソートされていると仮定する．
inline
ymuint
check_pat_list1(const vector<ymuint>& tv_list1,
		const vector<ymuint>& tv_list2)
{
  bool flag_10 = false;
  bool flag_01 = false;
  bool flag_11 = false;
  ymuint ans = 0U;
  ymuint n1 = tv_list1.size();
  ymuint n2 = tv_list2.size();
  ymuint p1 = 0;
  ymuint p2 = 0;
  ymuint v1 = tv_list1[p1];
  ymuint v2 = tv_list2[p2];
  for ( ; ; ) {
    if ( v1 < v2 ) {
      ans |= 1U;
      ++ p1;
      if ( p1 >= n1 ) {
	break;
      }
      v1 = tv_list1[p1];
    }
    else if ( v1 > v2 ) {
      ans |= 2U;
      ++ p2;
      if ( p2 >= n2 ) {
	break;
      }
      v2 = tv_list2[p2];
    }
    else {
      ++ p1;
      ++ p2;
      if ( p1 >= n1 ) {
	break;
      }
      v1 = tv_list1[p1];
      if ( p2 >= n2 ) {
	break;
      }
      v2 = tv_list2[p2];
    }
    if ( ans == 3U ) {
      return ans;
    }
  }
  if ( p1 < n1 ) {
    ans |= 1U;
  }
  if ( p2 < n2 ) {
    ans |= 2U;
  }
  return ans;
}

// パタン番号リストの包含関係を調べる．
// true  list1 と list2 に共通の要素がある．
//
// リストの内容は昇順にソートされていると仮定する．
inline
bool
check_pat_list2(const vector<ymuint>& tv_list1,
		const vector<ymuint>& tv_list2)
{
  bool flag_10 = false;
  bool flag_01 = false;
  bool flag_11 = false;
  ymuint n1 = tv_list1.size();
  ymuint n2 = tv_list2.size();
  ymuint p1 = 0;
  ymuint p2 = 0;
  ymuint v1 = tv_list1[p1];
  ymuint v2 = tv_list2[p2];
  for ( ; ; ) {
    if ( v1 < v2 ) {
      ++ p1;
      if ( p1 >= n1 ) {
	return false;
      }
      v1 = tv_list1[p1];
    }
    else if ( v1 > v2 ) {
      ++ p2;
      if ( p2 >= n2 ) {
	return false;
      }
      v2 = tv_list2[p2];
    }
    else {
      return true;
    }
  }
  return false;
}

void
mark_tfi(const TpgNode* node,
	 HashSet<ymuint>& tfi_mark,
	 vector<ymuint>& input_list)
{
  if ( tfi_mark.check(node->id()) ) {
    return;
  }
  tfi_mark.add(node->id());

  if ( node->is_input() ) {
    input_list.push_back(node->input_id());
  }
  else {
    ymuint ni = node->fanin_num();
    for (ymuint i = 0; i < ni; ++ i) {
      const TpgNode* inode = node->fanin(i);
      mark_tfi(inode, tfi_mark, input_list);
    }
  }
}

END_NONAMESPACE


//////////////////////////////////////////////////////////////////////
// クラス FaultAnalyzer
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
FaultAnalyzer::FaultAnalyzer()
{
  mVerbose = false;
}

// @brief デストラクタ
FaultAnalyzer::~FaultAnalyzer()
{
}

// @brief verbose フラグを設定する．
// @param[in] verbose 表示を制御するフラグ
void
FaultAnalyzer::set_verbose(int verbose)
{
  mVerbose = verbose;
}

// @brief 初期化する．
// @param[in] network ネットワーク
void
FaultAnalyzer::init(const TpgNetwork& network)
{
  StopWatch local_timer;
  local_timer.start();

  ymuint nn = network.active_node_num();
  mMaxNodeId = network.max_node_id();
  mMaxFaultId = 0;
  for (ymuint i = 0; i < nn; ++ i) {
    const TpgNode* node = network.active_node(i);
    if ( node->is_output() ) {
      continue;
    }
    ymuint nf = node->fault_num();
    for (ymuint i = 0; i < nf; ++ i) {
      TpgFault* fault = node->fault(i);
      ymuint f_id = fault->id();
      if ( mMaxFaultId < f_id ) {
	mMaxFaultId = f_id;
      }
    }
  }
  ++ mMaxFaultId;

  mInputListArray.clear();
  mInputListArray.resize(mMaxNodeId);

  mInputList2Array.clear();
  mInputList2Array.resize(mMaxNodeId);

  mNodeSetArray.clear();
  mNodeSetArray.resize(mMaxNodeId);

  mFaultInfoArray.clear();
  mFaultInfoArray.resize(mMaxFaultId);

  BackTracer* bt = new_BtJust2();
  bt->set_max_id(mMaxNodeId);

  ymuint f_all = 0;
  ymuint f_det = 0;
  ymuint f_red = 0;
  ymuint f_abt = 0;
  ymuint n_single_cube = 0;

  for (ymuint i = 0; i < nn; ++ i) {
    const TpgNode* node = network.active_node(i);
    if ( node->is_output() ) {
      continue;
    }

    NodeSet& node_set = mNodeSetArray[node->id()];
    node_set.mark_region(mMaxNodeId, node);

    vector<ymuint>& input_list = mInputListArray[node->id()];
    for (ymuint j = 0; j < node_set.tfo_tfi_size(); ++ j) {
      const TpgNode* node1 = node_set.tfo_tfi_node(j);
      if ( node1->is_input() ) {
	input_list.push_back(node1->input_id());
      }
    }
    // ソートしておく．
    sort(input_list.begin(), input_list.end());

    // 故障箇所の TFI に含まれる入力番号を mInputList2Array に入れる．
    HashSet<ymuint> tfi_mark;
    vector<ymuint>& input_list2 = mInputList2Array[node->id()];
    mark_tfi(node, tfi_mark, input_list2);
    // ソートしておく．
    sort(input_list2.begin(), input_list2.end());

    ymuint nf = node->fault_num();
    for (ymuint i = 0; i < nf; ++ i) {
      TpgFault* fault = node->fault(i);
      ymuint f_id = fault->id();
      FaultInfo& fi = mFaultInfoArray[f_id];
      fi.init(fault);

      ++ f_all;

      GvalCnf gval_cnf(mMaxNodeId);
      FvalCnf fval_cnf(mMaxNodeId, gval_cnf);
      SatEngine engine(string(), string(), NULL);

      // fault を検出するCNFを作る．
      fval_cnf.make_cnf(engine, fault, node_set, kVal1);

      // 故障に対するテスト生成を行なう．
      engine.assumption_begin();

      vector<Bool3> sat_model;
      SatStats sat_stats;
      USTime sat_time;
      fi.mStat = engine.solve(sat_model, sat_stats, sat_time);
      if ( fi.mStat == kB3True ) {
	++ f_det;
	mOrigFaultList.push_back(fault);
	ModelValMap val_map(fval_cnf.gvar_map(), fval_cnf.fvar_map(), sat_model);
	Extractor extract(val_map);
	NodeValList& suf_list = fi.mSufList;
	extract(fault, suf_list);
	(*bt)(node, node_set, val_map, fi.mPiSufList);

	// 必要割当を求める．
	NodeValList& ma_list = fi.mMaList;
	ymuint n = suf_list.size();
	for (ymuint i = 0; i < n; ++ i) {
	  NodeVal nv = suf_list[i];

	  NodeValList list1;
	  const TpgNode* node = nv.node();
	  bool val = nv.val();
	  list1.add(node, !val);
	  if ( engine.check_sat(gval_cnf, list1) == kB3False ) {
	    // node の値を反転したら検出できなかった．
	    // -> この割当は必須割当
	    ma_list.add(node, val);
	  }
	}
	if ( suf_list.size() == ma_list.size() ) {
	  fi.mSingleCube = true;
	  ++ n_single_cube;
	}
      }
      else if ( fi.mStat == kB3False ) {
	++ f_red;
      }
      else {
	++ f_abt;
      }
    }
  }

  mDomFaultList.clear();
  mDomFaultList.reserve(mOrigFaultList.size());
  for (ymuint i = 0; i < mOrigFaultList.size(); ++ i) {
    mDomFaultList.push_back(mOrigFaultList[i]);
  }

  delete bt;

  local_timer.stop();

  if ( mVerbose > 0 ) {
    cout << "Total " << setw(6) << f_all << " faults" << endl
	 << "Total " << setw(6) << f_det << " detected faults" << endl
	 << "     (" << setw(6) << n_single_cube << ") single cube assignment" << endl
	 << "Total " << setw(6) << f_red << " redundant faults" << endl
	 << "Total " << setw(6) << f_abt << " aborted faults" << endl
	 << "CPU time " << local_timer.time() << endl
	 << endl;
  }
}

// @brief 故障シミュレーションを行い，故障検出パタンを記録する．
// @param[in] fsim 故障シミュレータ
// @param[in] tvmgr テストベクタのマネージャ
// @param[in] tv_list テストベクタのリスト
// @param[in] rg 乱数生成器
// @param[in] npat 内部で生成する乱数パタンの数
//
// 結果は mFaultInfoArray の pat_list に格納される．
void
FaultAnalyzer::get_pat_list(Fsim& fsim,
			    TvMgr& tvmgr,
			    const vector<TestVector*>& tv_list,
			    RandGen& rg)
{
  StopWatch local_timer;
  local_timer.start();

  if ( mVerbose ) {
    cout << "  fault simulation (npat = " << tv_list.size() << ") starts."
	 << endl;
  }

  vector<TestVector*> cur_array;
  cur_array.reserve(kPvBitLen);

  KDet2Op op(fsim, mOrigFaultList);

  ymuint npat = tv_list.size();
  ymuint base = 0;
  for (ymuint i = 0; i < tv_list.size(); ++ i) {
    TestVector* tv = tv_list[i];
    cur_array.push_back(tv);
    if ( cur_array.size() == kPvBitLen ) {
      if ( mVerbose > 1 ) {
	cout << "\r " << base;
	cout.flush();
      }
      fsim.ppsfp(cur_array, op);
      for (ymuint j = 0; j < kPvBitLen; ++ j) {
	const vector<ymuint>& det_list = op.det_list(j);
	record_pat(det_list, base + j);
      }
      cur_array.clear();
      op.clear_det_list();
      base += kPvBitLen;
    }
  }
  if ( !cur_array.empty() ) {
    fsim.ppsfp(cur_array, op);
    for (ymuint j = 0; j < cur_array.size(); ++ j) {
      const vector<ymuint>& det_list = op.det_list(j);
      record_pat(det_list, base + j);
    }
    op.clear_det_list();
    base += cur_array.size();
    cur_array.clear();
  }

  for (ymuint i = 0; i < kPvBitLen; ++ i) {
    TestVector* tv = tvmgr.new_vector();
    cur_array.push_back(tv);
  }
  ymuint nochg_count = 0;
  for ( ; ; ) {
    for (ymuint i = 0; i < kPvBitLen; ++ i) {
      cur_array[i]->set_from_random(rg);
    }
    fsim.ppsfp(cur_array, op);
    ymuint nchg = 0;
    for (ymuint j = 0; j < kPvBitLen; ++ j) {
      const vector<ymuint>& det_list = op.det_list(j);
      nchg += record_pat(det_list, base + j);
    }
    cur_array.clear();
    op.clear_det_list();
    base += kPvBitLen;
    if ( nchg == 0 ) {
      ++ nochg_count;
      if ( nochg_count > 3 ) {
	break;
      }
    }
    else {
      nochg_count = 0;
    }
    if ( mVerbose > 1 ) {
      cout << "\r " << base;
      cout.flush();
    }
  }
  if ( mVerbose > 1 ) {
    cout << endl;
  }

  // 乱数パタンは削除しておく．
  for (ymuint i = 0; i < kPvBitLen; ++ i) {
    tvmgr.delete_vector(cur_array[i]);
  }

  // FaultInfo::mDomCandList の後始末．
  for (ymuint i = 0; i < mOrigFaultList.size(); ++ i) {
    TpgFault* fault = mOrigFaultList[i];
    FaultInfo& fi = mFaultInfoArray[fault->id()];
    if ( fi.mDomCandList.size() != fi.mDomCandListSize ) {
      fi.mDomCandList.erase(fi.mDomCandList.begin() + fi.mDomCandListSize, fi.mDomCandList.end());
    }
  }

  local_timer.stop();

  if ( mVerbose ) {
    cout << "  fault simulation ends." << endl
	 << "CPU time (fault simulation)  " << local_timer.time() << endl
	 << "Total " << base << " patterns simulated" << endl;
  }

}

ymuint
FaultAnalyzer::record_pat(const vector<ymuint>& det_list,
			  ymuint pat_id)
{
  ymuint n = det_list.size();
  ymuint nchg = 0;
  vector<bool> det_flag(mMaxFaultId, false);
  for (ymuint i = 0; i < n; ++ i) {
    ymuint f_id = det_list[i];
    det_flag[f_id] = true;
    FaultInfo& fi = mFaultInfoArray[f_id];
    fi.add_pat(pat_id);
    fi.add_fnum(n);
  }
  // 検出結果を用いて支配される故障の候補リストを作る．
  for (ymuint i = 0; i < n; ++ i) {
    ymuint f_id = det_list[i];
    FaultInfo& fi = mFaultInfoArray[f_id];
    if ( fi.mFirstDetect ) {
      fi.mFirstDetect = false;
      fi.mDomCandList.reserve(n - 1);
      // 初めて検出された場合
      // 構造的に独立でない故障を候補にする．
      TpgNode* f1node = fi.fault()->node();
      ymuint f1node_id = f1node->id();
      const vector<ymuint>& input_list1 = mInputListArray[f1node_id];
      for (ymuint j = 0; j < n; ++ j) {
	ymuint f_id2 = det_list[j];
	if ( f_id2 == f_id ) {
	  continue;
	}
	TpgNode* f2node = mFaultInfoArray[f_id2].fault()->node();
	ymuint f2node_id = f2node->id();
	const vector<ymuint>& input_list2 = mInputListArray[f2node_id];
	bool intersect = check_intersect(input_list1, input_list2);
	if ( !intersect ) {
	  // 共通部分を持たない故障は独立
	  continue;
	}
	fi.mDomCandList.push_back(f_id2);
      }
      fi.mDomCandListSize = fi.mDomCandList.size();
    }
    else {
      // 二回目以降
      // 候補のうち，今回検出されなかった故障を外す．
      ymuint wpos = 0;
      for (ymuint j = 0; j < fi.mDomCandListSize; ++ j) {
	ymuint f_id2 = fi.mDomCandList[j];
	if ( det_flag[f_id2] ) {
	  if ( wpos != j ) {
	    fi.mDomCandList[wpos] = f_id2;
	  }
	  ++ wpos;
	}
      }
      if ( wpos < fi.mDomCandListSize ) {
	nchg += fi.mDomCandListSize - wpos;
	fi.mDomCandListSize = wpos;
      }
    }
  }
  return nchg;
}

#if 0
// @brief 支配故障を求める．
//
// 結果は mDomFaultList に格納される．
void
FaultAnalyzer::get_dom_faults(bool fast)
{
  StopWatch local_timer;

  local_timer.start();

  vector<bool> dom_flag(mMaxFaultId, false);

  ymuint fault_num = mOrigFaultList.size();

  ymuint n_sat2 = 0;
  ymuint n_dom2 = 0;
  ymuint n_pcheck1 = 0;
  ymuint n_pcheck2 = 0;

  vector<vector<TpgFault*> > pending_list(mMaxFaultId);

  // 故障を検出パタン数の少ない順に並べる．
  vector<ymuint> fault_list(fault_num);
  for (ymuint i = 0; i < fault_num; ++ i) {
    fault_list[i] = mOrigFaultList[i]->id();
  }

  FaultLt fault_lt(mFaultInfoArray);
  sort(fault_list.begin(), fault_list.end(), fault_lt);

  // シミュレーション結果から故障の支配関係のヒントを作り，
  // SAT で正確に判定する．
  for (ymuint i1 = 0; i1 < fault_num; ++ i1) {
    ymuint f1_id = fault_list[i1];
    if ( dom_flag[f1_id] ) {
      continue;
    }
    FaultInfo& fi1 = mFaultInfoArray[f1_id];
    TpgFault* f1 = fi1.fault();
    if ( mVerbose > 1 ) {
      cout << "\r                  ";
      cout << "\r" << setw(6) << i1 << " / " << setw(6) << fault_num;
      cout.flush();
    }

    TpgNode* f1node = f1->node();
    ymuint f1node_id = f1node->id();

    const vector<ymuint>& input_list1 = mInputListArray[f1node_id];
    const vector<ymuint>& input_list1_2 = mInputList2Array[f1node_id];
    const vector<ymuint>& pat_list1 = fi1.pat_list();

    for (ymuint i2 = i1 + 1; i2 < fault_num; ++ i2) {
      ymuint f2_id = fault_list[i2];
      if ( dom_flag[f2_id] ) {
	continue;
      }
      FaultInfo& fi2 = mFaultInfoArray[f2_id];
      TpgFault* f2 = fi2.fault();

      TpgNode* f2node = f2->node();
      ymuint f2node_id = f2node->id();

      const vector<ymuint>& input_list2 = mInputListArray[f2node_id];
      bool intersect = check_intersect(input_list1, input_list2);
      if ( !intersect ) {
	// 共通部分を持たない故障は独立
	continue;
      }

      const vector<ymuint>& input_list2_2 = mInputList2Array[f2node_id];
      bool intersect2 = check_intersect(input_list1_2, input_list2_2);
      if ( !intersect2 ) {
	// TFI が共通部分を持たない場合は望みが薄いので後回し．
	if ( !fast ) {
	  pending_list[f1_id].push_back(f2);
	}
	continue;
      }

      const vector<ymuint>& pat_list2 = fi2.pat_list();

      ++ n_pcheck1;
      ymuint stat = check_pat_list1(pat_list1, pat_list2);
      if ( stat == 3U ) {
	continue;
      }

      NodeSet& node_set2 = mNodeSetArray[f2node_id];

      if ( (stat & 1U) == 0U ) {
	// f1 が 0 のときは f2 も 0 だった．
	++ n_sat2;
	if ( check_fault_dominance(f1, f2) ) {
	  //cout << f2->str() << " is dominated by " << f1->str() << endl;
	  dom_flag[f2_id] = true;
	  ++ n_dom2;
	  continue;
	}
      }
      if ( (stat & 2U) == 0U ) {
	// f2 が 0 のときは f1 も 0 だった．
	++ n_sat2;
	if ( check_fault_dominance(f2, f1) ) {
	  //cout << f1->str() << " is dominated by " << f2->str() << endl;
	  dom_flag[f1_id] = true;
	  ++ n_dom2;
	  break;
	}
      }
    }
  }
  vector<TpgFault*> fault_list2;
  fault_list2.reserve(fault_num);
  for (ymuint i = 0; i < fault_num; ++ i) {
    TpgFault* f = mOrigFaultList[i];
    if ( !dom_flag[f->id()] ) {
      fault_list2.push_back(f);
    }
  }
  ymuint fault_num2 = fault_list2.size();
  if ( mVerbose ) {
    cout << " --> " << setw(6) << fault_num2 << ": " << setw(6) << n_dom2 << " / " << setw(6) << n_sat2
	 << ": Lap CPU time " << local_timer.time() << endl;
  }

  ymuint n_sat3 = 0;
  ymuint n_dom3 = 0;

  // シミュレーション結果から故障の支配関係のヒントを作り，
  // SAT で正確に判定する．
  for (ymuint i1 = 0; i1 < fault_num2; ++ i1) {
    TpgFault* f1 = fault_list2[i1];
    ymuint f1_id = f1->id();

    if ( mVerbose > 1 ) {
      cout << "\r                  ";
      cout << "\r" << setw(6) << i1 << " / " << setw(6) << fault_num2;
      cout.flush();
    }

    // f1 を検出するパタンのリスト
    const vector<ymuint>& pat_list1 = mFaultInfoArray[f1_id].pat_list();

    const vector<TpgFault*>& f_list = pending_list[f1_id];
    for (ymuint i2 = 0; i2 < f_list.size(); ++ i2) {
      TpgFault* f2 = f_list[i2];
      ymuint f2_id = f2->id();
      if ( dom_flag[f2_id] ) {
	continue;
      }

      const vector<ymuint>& pat_list2 = mFaultInfoArray[f2_id].pat_list();

      ++ n_pcheck2;
      ymuint stat = check_pat_list1(pat_list1, pat_list2);
      if ( stat == 3U ) {
	continue;
      }

      if ( (stat & 1U) == 0U ) {
	// f1 が 0 のときは f2 も 0 だった．
	++ n_sat3;
	if ( check_fault_dominance(f1, f2) ) {
	  //cout << f2->str() << " is dominated by " << f1->str() << endl;
	  dom_flag[f2_id] = true;
	  ++ n_dom3;
	  continue;
	}
      }
      if ( (stat & 2U) == 0U ) {
	// f2 が 0 のときは f1 も 0 だった．
	++ n_sat3;
	if ( check_fault_dominance(f2, f1) ) {
	  //cout << f1->str() << " is dominated by " << f2->str() << endl;
	  dom_flag[f1_id] = true;
	  ++ n_dom3;
	  break;
	}
      }
    }
  }

  // 支配されていない故障を mDomFaultList に入れる．
  mDomFaultList.clear();
  mDomFaultList.reserve(fault_num2);
  for (ymuint i = 0; i < fault_num2; ++ i) {
    TpgFault* f = fault_list2[i];
    if ( !dom_flag[f->id()] ) {
      mDomFaultList.push_back(f);
    }
  }

  ymuint dom_fault_num = mDomFaultList.size();
  local_timer.stop();
  if ( mVerbose ) {
    cout << " --> " << setw(6) << dom_fault_num << ": " << setw(6) << n_dom3 << " / " << setw(6) << n_sat3 << endl;
    cout << "Total    " << fault_num << " original faults" << endl;
    cout << "Total    " << dom_fault_num << " dominator faults" << endl;
    cout << "Total    " << n_sat2 + n_sat3 << " dominance test" << endl;
    cout << "Total    " << n_pcheck1 << " + " << n_pcheck2 << " pattern check" << endl;
    cout << "CPU time for dominance test" << local_timer.time() << endl;
  }
}

#else

// @brief 支配故障を求める．
//
// 結果は mDomFaultList に格納される．
void
FaultAnalyzer::get_dom_faults()
{
  StopWatch local_timer;

  local_timer.start();

  vector<bool> dom_flag(mMaxFaultId, false);

  ymuint fault_num = mOrigFaultList.size();

  ymuint n_sat2 = 0;
  ymuint n_dom2 = 0;

  vector<ymuint> pending_f1_list;
  vector<vector<ymuint> > pending_list_array(mMaxFaultId);

  // 故障を支配故障候補数の少ない順に並べる．
  vector<ymuint> fault_list(fault_num);
  for (ymuint i = 0; i < fault_num; ++ i) {
    fault_list[i] = mOrigFaultList[i]->id();
  }

  FaultLt fault_lt(mFaultInfoArray);
  sort(fault_list.begin(), fault_list.end(), fault_lt);

  ymuint cur_num = fault_num;

  // シミュレーション結果から故障の支配関係のヒントを作り，
  // SAT で正確に判定する．
  for (ymuint i1 = 0; i1 < fault_num; ++ i1) {
    ymuint f1_id = fault_list[i1];
    if ( dom_flag[f1_id] ) {
      continue;
    }

    FaultInfo& fi1 = mFaultInfoArray[f1_id];
    const vector<ymuint>& cand_list = fi1.mDomCandList;
    if ( cand_list.empty() ) {
      continue;
    }

    if ( mVerbose > 1 ) {
      cout << "\r                  ";
      cout << "\r" << setw(6) << i1 << " / " << setw(6) << fault_num
	   << " / " << setw(6) << cur_num;
      cout.flush();
    }

    TpgFault* f1 = fi1.fault();
    TpgNode* f1node = f1->node();
    ymuint f1node_id = f1node->id();
    const vector<ymuint>& input_list1_2 = mInputList2Array[f1node_id];

    bool pending = false;
    for (ymuint i2 = 0; i2 < cand_list.size(); ++ i2) {
      ymuint f2_id = cand_list[i2];
      if ( dom_flag[f2_id] ) {
	continue;
      }

      FaultInfo& fi2 = mFaultInfoArray[f2_id];
      TpgFault* f2 = fi2.fault();
      TpgNode* f2node = f2->node();
      ymuint f2node_id = f2node->id();
      const vector<ymuint>& input_list2_2 = mInputList2Array[f2node_id];

      bool intersect2 = check_intersect(input_list1_2, input_list2_2);
      if ( !intersect2 ) {
	// TFI が共通部分を持たない場合は望みが薄いので後回し．
	pending_list_array[f1_id].push_back(f2_id);
	if ( !pending ) {
	  pending = true;
	  pending_f1_list.push_back(f1_id);
	}
	continue;
      }

      // 実際にチェックを行う．
      ++ n_sat2;
      if ( check_fault_dominance(f1, f2) ) {
	//cout << f2->str() << " is dominated by " << f1->str() << endl;
	dom_flag[f2_id] = true;
	++ n_dom2;
	-- cur_num;
	continue;
      }
    }
  }
  vector<TpgFault*> fault_list2;
  fault_list2.reserve(fault_num);
  for (ymuint i = 0; i < fault_num; ++ i) {
    TpgFault* f = mOrigFaultList[i];
    if ( !dom_flag[f->id()] ) {
      fault_list2.push_back(f);
    }
  }
  ymuint fault_num2 = fault_list2.size();
  if ( mVerbose ) {
    cout << " --> " << setw(6) << fault_num2 << ": " << setw(6) << n_dom2 << " / " << setw(6) << n_sat2
	 << ": Lap CPU time " << local_timer.time() << endl;
  }

  ymuint n_sat3 = 0;
  ymuint n_dom3 = 0;

  // シミュレーション結果から故障の支配関係のヒントを作り，
  // SAT で正確に判定する．
  for (ymuint i1 = 0; i1 < pending_f1_list.size(); ++ i1) {
    ymuint f1_id = pending_f1_list[i1];
    if ( dom_flag[f1_id] ) {
      continue;
    }
    if ( mVerbose > 1 ) {
      cout << "\r                  ";
      cout << "\r" << setw(6) << i1 << " / " << setw(6) << pending_f1_list.size()
	   << " / " << setw(6) << cur_num;
      cout.flush();
    }

    FaultInfo& fi1 = mFaultInfoArray[f1_id];
    TpgFault* f1 = fi1.fault();

    const vector<ymuint>& f_list = pending_list_array[f1_id];
    for (ymuint i2 = 0; i2 < f_list.size(); ++ i2) {
      ymuint f2_id = f_list[i2];
      if ( dom_flag[f2_id] ) {
	continue;
      }

      FaultInfo& fi2 = mFaultInfoArray[f2_id];
      TpgFault* f2 = fi2.fault();

      ++ n_sat3;
      if ( check_fault_dominance(f1, f2) ) {
	//cout << f2->str() << " is dominated by " << f1->str() << endl;
	dom_flag[f2_id] = true;
	++ n_dom3;
	-- cur_num;
	continue;
      }
    }
  }

  // 支配されていない故障を mDomFaultList に入れる．
  mDomFaultList.clear();
  mDomFaultList.reserve(fault_num2);
  for (ymuint i = 0; i < fault_num2; ++ i) {
    TpgFault* f = fault_list2[i];
    if ( !dom_flag[f->id()] ) {
      mDomFaultList.push_back(f);
    }
  }

  ymuint dom_fault_num = mDomFaultList.size();
  local_timer.stop();
  if ( mVerbose ) {
    cout << " --> " << setw(6) << dom_fault_num << ": " << setw(6) << n_dom3 << " / " << setw(6) << n_sat3 << endl;
    cout << "Total    " << fault_num << " original faults" << endl;
    cout << "Total    " << dom_fault_num << " dominator faults" << endl;
    cout << "Total    " << n_sat2 + n_sat3 << " dominance test" << endl;
    cout << "CPU time for dominance test" << local_timer.time() << endl;
  }
}
#endif

// @brief 十分割当と必要割当を求める．
//
// 結果は mFaultInfoArray に格納される．
void
FaultAnalyzer::analyze_faults()
{
  StopWatch local_timer;

  local_timer.start();

  ymuint n_exact = 0;
  ymuint fnum = mDomFaultList.size();
  for (ymuint i = 0; i < fnum; ++ i) {
    if ( mVerbose > 1 ) {
      cout << "\r                  ";
      cout << "\r" << setw(6) << i << " / " << setw(6) << fnum;
      cout.flush();
    }

    TpgFault* fault = mDomFaultList[i];
    FaultInfo& fault_info = mFaultInfoArray[fault->id()];

    TpgCnf1 tpg_cnf(string(), string(), NULL);

    tpg_cnf.make_fval_cnf(fault, mMaxNodeId);

    NodeValList& suf_list = fault_info.mSufList;
    NodeValList& pi_suf_list = fault_info.mPiSufList;
    NodeValList& ma_list = fault_info.mMaList;;

    bool stat = tpg_cnf.get_pi_suf_list(suf_list, pi_suf_list);
    ASSERT_COND( stat );

    // 必要割当を求める．
    ymuint n = suf_list.size();
    for (ymuint i = 0; i < n; ++ i) {
      NodeVal nv = suf_list[i];

      NodeValList list1;
      const TpgNode* node = nv.node();
      bool val = nv.val();
      list1.add(node, !val);
      if ( tpg_cnf.check_conflict(list1) ) {
	ma_list.add(node, val);
      }
    }
    if ( suf_list.size() == ma_list.size() ) {
      fault_info.mSingleCube = true;
      ++ n_exact;
    }
  }

  local_timer.stop();

  if ( mVerbose > 0 ) {
    if ( mVerbose > 1 ) {
      cout << endl;
    }
    cout << "CPU time for fault analysis" << local_timer.time() << endl;
    cout << "  " << n_exact << " exact faults" << endl;
  }
}

// @brief 故障間の衝突性を調べる．
void
FaultAnalyzer::analyze_conflict()
{
  StopWatch local_timer;
  local_timer.start();

  mConflictStats.conf_timer.reset();
  mConflictStats.conf1_timer.reset();
  mConflictStats.conf2_timer.reset();
  mConflictStats.conf3_timer.reset();
  mConflictStats.conf4_timer.reset();
  mConflictStats.int1_timer.reset();
  mConflictStats.int2_timer.reset();
  mConflictStats.conf1_count = 0;
  mConflictStats.conf2_count = 0;
  mConflictStats.conf3_count = 0;
  mConflictStats.conf4_count = 0;
  mConflictStats.conf4_check_count = 0;
  mConflictStats.int1_count = 0;
  mConflictStats.int2_count = 0;

  // シミュレーション結果を用いてコンフリクトチェックのスクリーニングを行う．
  ymuint fault_num = mDomFaultList.size();
  for (ymuint i1 = 0; i1 < fault_num; ++ i1) {
    TpgFault* f1 = mDomFaultList[i1];

    if ( mVerbose > 1 ) {
      cout << "\r                  ";
      cout << "\r" << setw(6) << i1 << " / " << setw(6) << fault_num;
      cout.flush();
    }
    vector<TpgFault*> f2_list;
    f2_list.reserve(fault_num - i1 - 1);
    for (ymuint i2 = i1 + 1; i2 < fault_num; ++ i2) {
      TpgFault* f2 = mDomFaultList[i2];
      f2_list.push_back(f2);
    }
    vector<TpgFault*> conf_list;
    analyze_conflict(f1, f2_list, conf_list, false, false);
    for (ymuint i = 0; i < conf_list.size(); ++ i) {
      TpgFault* f2 = conf_list[i];
      mFaultInfoArray[f1->id()].mConflictList.push_back(f2->id());
      mFaultInfoArray[f2->id()].mConflictList.push_back(f1->id());
    }
  }

  for (ymuint i1 = 0; i1 < fault_num; ++ i1) {
    TpgFault* f1 = mDomFaultList[i1];
    FaultInfo& fi = mFaultInfoArray[f1->id()];
    sort(fi.mConflictList.begin(), fi.mConflictList.end());
  }

  local_timer.stop();

  if ( mVerbose ) {
    cout << endl;
    print_conflict_stats(cout);
    cout << "Total CPU time " << local_timer.time() << endl;
  }
}

// @brief 故障間の衝突性を調べる．
void
FaultAnalyzer::estimate_conflict(ymuint sample_num,
				 vector<double>& conf_prob_array)
{
  StopWatch local_timer;
  local_timer.start();

  mConflictStats.conf_timer.reset();
  mConflictStats.conf1_timer.reset();
  mConflictStats.conf2_timer.reset();
  mConflictStats.conf3_timer.reset();
  mConflictStats.conf4_timer.reset();
  mConflictStats.int1_timer.reset();
  mConflictStats.int2_timer.reset();
  mConflictStats.conf_count = 0;
  mConflictStats.conf1_count = 0;
  mConflictStats.conf2_count = 0;
  mConflictStats.conf3_count = 0;
  mConflictStats.conf4_count = 0;
  mConflictStats.conf4_check_count = 0;
  mConflictStats.int1_count = 0;
  mConflictStats.int2_count = 0;

  RandGen rg;

  conf_prob_array.clear();
  conf_prob_array.resize(mMaxFaultId);

  // シミュレーション結果を用いてコンフリクトチェックのスクリーニングを行う．
  ymuint fault_num = mDomFaultList.size();
  for (ymuint i1 = 0; i1 < fault_num; ++ i1) {
    TpgFault* f1 = mDomFaultList[i1];

    if ( mVerbose > 1 ) {
      cout << "\r                  ";
      cout << "\r" << setw(6) << i1 << " / " << setw(6) << fault_num;
      cout.flush();
    }
    vector<TpgFault*> f2_list;
    if ( sample_num < fault_num ) {
      f2_list.reserve(sample_num);
      for (ymuint i2 = 0; i2 < sample_num; ++ i2) {
	ymuint pos = rg.int32() % fault_num;
	TpgFault* f2 = mDomFaultList[pos];
	f2_list.push_back(f2);
      }
    }
    else {
      f2_list.reserve(fault_num);
      for (ymuint i2 = 0; i2 < fault_num; ++ i2) {
	TpgFault* f2 = mDomFaultList[i2];
	f2_list.push_back(f2);
      }
    }
    vector<TpgFault*> conf_list;
    analyze_conflict(f1, f2_list, conf_list, true, false);
    conf_prob_array[f1->id()] = static_cast<double>(conf_list.size()) / sample_num;
  }

  local_timer.stop();

  if ( mVerbose > 1 ) {
    cout << endl;
  }
  if ( mVerbose > 0 ) {
    print_conflict_stats(cout);
    cout << "Total CPU time " << local_timer.time() << endl;
  }
}

// @brief 1つの故障と複数の故障間の衝突性を調べる．
void
FaultAnalyzer::analyze_conflict(TpgFault* f1,
				const vector<TpgFault*>& f2_list,
				vector<TpgFault*>& conf_list,
				bool simple,
				bool local_verbose)
{
  if ( mFaultInfoArray[f1->id()].mSingleCube ) {
    analyze_conflict2(f1, f2_list, conf_list, simple, local_verbose);
    return;
  }

  mConflictStats.conf_timer.start();

  GvalCnf gval_cnf(mMaxNodeId);
  FvalCnf fval_cnf(mMaxNodeId, gval_cnf);
  SatEngine engine(string(), string(), NULL);

  fval_cnf.make_cnf(engine, f1, kVal1);

  const vector<ymuint>& input_list1 = mInputListArray[f1->node()->id()];

  FaultInfo& fi1 = mFaultInfoArray[f1->id()];
  const vector<ymuint>& tv_list1 = fi1.pat_list();
  const NodeValList& suf_list1 = fi1.mSufList;
  const NodeValList& pi_suf_list1 = fi1.mPiSufList;
  const NodeValList& ma_list1 = fi1.mMaList;

  conf_list.reserve(f2_list.size());
  for (ymuint i2 = 0; i2 < f2_list.size(); ++ i2) {
    TpgFault* f2 = f2_list[i2];

    const vector<ymuint>& input_list2 = mInputListArray[f2->node()->id()];

    bool intersect = check_intersect(input_list1, input_list2);
    if ( !intersect ) {
      // 共通部分を持たない故障は独立
      continue;
    }

    FaultInfo& fi2 = mFaultInfoArray[f2->id()];
    const vector<ymuint>& tv_list2 = fi2.pat_list();

    bool stat = check_pat_list2(tv_list1, tv_list2);
    if ( stat ) {
      // f1 と f2 が同時に1なのでコンフリクトしない．
      continue;
    }

    const NodeValList& suf_list2 = fi2.mSufList;
    const NodeValList& pi_suf_list2 = fi2.mPiSufList;
    const NodeValList& ma_list2 = fi2.mMaList;

    mConflictStats.conf1_timer.start();
    if ( check_conflict(ma_list1, ma_list2) ) {
      // 必要割当そのものがコンフリクトしている．
      ++ mConflictStats.conf_count;
      ++ mConflictStats.conf1_count;
      conf_list.push_back(f2);
      mConflictStats.conf1_timer.stop();
      continue;
    }
    mConflictStats.conf1_timer.stop();


    mConflictStats.int1_timer.start();
    if ( !check_conflict(pi_suf_list1, pi_suf_list2) ) {
      // 十分割当が両立しているのでコンフリクトしない．
      ++ mConflictStats.int1_count;
      mConflictStats.int1_timer.stop();
      continue;
    }
    mConflictStats.int1_timer.stop();

    mConflictStats.int2_timer.start();
    Bool3 sat_stat = engine.check_sat(gval_cnf, suf_list2);
    if ( sat_stat == kB3True ) {
      // f2 の十分割当のもとで f1 が検出できれば f1 と f2 はコンフリクトしない．
      ++ mConflictStats.int2_count;
      mConflictStats.int2_timer.stop();
      continue;
    }
    mConflictStats.int2_timer.stop();

    if ( fi2.mSingleCube ) {
      if ( sat_stat == kB3False ) {
	++ mConflictStats.conf_count;
	++ mConflictStats.conf3_count;
	conf_list.push_back(f2);
      }
      // f2 の十分割当と必要割当が等しければ上のチェックで終わり．
      continue;
    }

    mConflictStats.conf3_timer.start();
    if ( engine.check_sat(gval_cnf, ma_list2) == kB3False ) {
      // f2 の必要割当のもとで f1 が検出できなければ f1 と f2 はコンフリクトしている．
      ++ mConflictStats.conf_count;
      ++ mConflictStats.conf3_count;
      conf_list.push_back(f2);
      mConflictStats.conf3_timer.stop();
      continue;
    }
    mConflictStats.conf3_timer.stop();

    if ( simple ) {
      continue;
    }

    mConflictStats.conf4_timer.start();
    ++ mConflictStats.conf4_check_count;
    if ( check_fault_conflict(f1, f2) ) {
      ++ mConflictStats.conf_count;
      ++ mConflictStats.conf4_count;
      conf_list.push_back(f2);
    }
    mConflictStats.conf4_timer.stop();
  }

  mConflictStats.conf_timer.stop();

  if ( mVerbose > 0 && local_verbose ) {
    if ( mVerbose > 1 ) {
      cout << endl;
    }
    print_conflict_stats(cout);
  }
}

// @brief 1つの故障と複数の故障間の衝突性を調べる．
void
FaultAnalyzer::analyze_conflict2(TpgFault* f1,
				 const vector<TpgFault*>& f2_list,
				 vector<TpgFault*>& conf_list,
				 bool simple,
				 bool local_verbose)
{
  mConflictStats.conf_timer.start();

  GvalCnf gval_cnf(mMaxNodeId);
  SatEngine engine(string(), string(), NULL);

  const vector<ymuint>& input_list1 = mInputListArray[f1->node()->id()];

  FaultInfo& fi1 = mFaultInfoArray[f1->id()];
  const vector<ymuint>& tv_list1 = fi1.pat_list();
  const NodeValList& suf_list1 = fi1.mSufList;
  const NodeValList& pi_suf_list1 = fi1.mPiSufList;

  conf_list.reserve(f2_list.size());
  for (ymuint i2 = 0; i2 < f2_list.size(); ++ i2) {
    TpgFault* f2 = f2_list[i2];

    const vector<ymuint>& input_list2 = mInputListArray[f2->node()->id()];

    bool intersect = check_intersect(input_list1, input_list2);
    if ( !intersect ) {
      // 共通部分を持たない故障は独立
      continue;
    }

    FaultInfo& fi2 = mFaultInfoArray[f2->id()];
    const vector<ymuint>& tv_list2 = fi2.pat_list();

    bool stat = check_pat_list2(tv_list1, tv_list2);
    if ( stat ) {
      // f1 と f2 が同時に1なのでコンフリクトしない．
      continue;
    }

    const NodeValList& suf_list2 = fi2.mSufList;
    const NodeValList& pi_suf_list2 = fi2.mPiSufList;
    const NodeValList& ma_list2 = fi2.mMaList;

    mConflictStats.conf1_timer.start();
    if ( check_conflict(suf_list1, ma_list2) ) {
      // 必要割当そのものがコンフリクトしている．
      ++ mConflictStats.conf_count;
      ++ mConflictStats.conf1_count;
      conf_list.push_back(f2);
      mConflictStats.conf1_timer.stop();
      continue;
    }
    mConflictStats.conf1_timer.stop();


    mConflictStats.int1_timer.start();
    if ( !check_conflict(pi_suf_list1, pi_suf_list2) ) {
      // 十分割当が両立しているのでコンフリクトしない．
      ++ mConflictStats.int1_count;
      mConflictStats.int1_timer.stop();
      continue;
    }
    mConflictStats.int1_timer.stop();

    mConflictStats.int2_timer.start();
    Bool3 sat_stat = engine.check_sat(gval_cnf, suf_list1, suf_list2);
    if ( sat_stat == kB3True ) {
      // f2 の十分割当のもとで f1 が検出できれば f1 と f2 はコンフリクトしない．
      ++ mConflictStats.int2_count;
      mConflictStats.int2_timer.stop();
      continue;
    }
    mConflictStats.int2_timer.stop();

    if ( fi2.mSingleCube ) {
      if ( sat_stat == kB3False ) {
	++ mConflictStats.conf_count;
	++ mConflictStats.conf3_count;
	conf_list.push_back(f2);
      }
      // f2 の十分割当と必要割当が等しければ上のチェックで終わり．
      continue;
    }

    mConflictStats.conf3_timer.start();
    if ( engine.check_sat(gval_cnf, suf_list1, ma_list2) == kB3False ) {
      // f2 の必要割当のもとで f1 が検出できなければ f1 と f2 はコンフリクトしている．
      ++ mConflictStats.conf_count;
      ++ mConflictStats.conf3_count;
      conf_list.push_back(f2);
      mConflictStats.conf3_timer.stop();
      continue;
    }
    mConflictStats.conf3_timer.stop();

    if ( simple ) {
      continue;
    }

    mConflictStats.conf4_timer.start();
    ++ mConflictStats.conf4_check_count;
    {
      GvalCnf gval_cnf(mMaxNodeId);
      FvalCnf fval_cnf(mMaxNodeId, gval_cnf);
      SatEngine engine(string(), string(), NULL);
      fval_cnf.make_cnf(engine, f2, kVal1);
      if ( engine.check_sat(gval_cnf, suf_list1) == kB3False ) {
	++ mConflictStats.conf_count;
	++ mConflictStats.conf4_count;
	conf_list.push_back(f2);
      }
    }
    mConflictStats.conf4_timer.stop();
  }

  mConflictStats.conf_timer.stop();

  if ( mVerbose > 0 && local_verbose ) {
    if ( mVerbose > 1 ) {
      cout << endl;
    }
    print_conflict_stats(cout);
  }
}

// @brief 衝突リストを得る．
void
FaultAnalyzer::get_conf_list(vector<pair<ymuint, ymuint> >& conf_list)
{
  vector<ymuint> rmap(mMaxFaultId);
  for (ymuint i = 0; i < mDomFaultList.size(); ++ i) {
    TpgFault* f1 = mDomFaultList[i];
    rmap[f1->id()] = i;
  }
  for (ymuint i = 0; i < mDomFaultList.size(); ++ i) {
    TpgFault* f1 = mDomFaultList[i];
    FaultInfo& fi1 = mFaultInfoArray[f1->id()];
    const vector<ymuint>& conf_list1 = fi1.mConflictList;
    for (ymuint j = 0; j < conf_list1.size(); ++ j) {
      ymuint id2 = conf_list1[j];
      conf_list.push_back(make_pair(i, rmap[id2]));
    }
  }
}

// @brief 支配故障リストを得る．
const vector<TpgFault*>&
FaultAnalyzer::dom_fault_list() const
{
  return mDomFaultList;
}

// @brief 故障の情報を得る．
const vector<FaultInfo>&
FaultAnalyzer::fault_info_array() const
{
  return mFaultInfoArray;
}

// @brief 個別の故障の情報を得る．
// @param[in] fid 故障番号
const FaultInfo&
FaultAnalyzer::fault_info(ymuint fid) const
{
  ASSERT_COND( fid < mMaxFaultId );
  return mFaultInfoArray[fid];
}

// @brief 入力番号リストを得る．
const vector<vector<ymuint> >&
FaultAnalyzer::input_list_array() const
{
  return mInputListArray;
}

// @brief f1 が f2 を支配しているか調べる．
bool
FaultAnalyzer::check_fault_dominance(TpgFault* f1,
				     TpgFault* f2)
{
  GvalCnf gval_cnf(mMaxNodeId);
  FvalCnf fval_cnf1(mMaxNodeId, gval_cnf);
  FvalCnf fval_cnf2(mMaxNodeId, gval_cnf);
  SatEngine engine(string(), string(), NULL);

  NodeSet& node_set1 = mNodeSetArray[f1->node()->id()];
  NodeSet& node_set2 = mNodeSetArray[f2->node()->id()];

  fval_cnf1.make_cnf(engine, f1, node_set1, kVal1);
  fval_cnf2.make_cnf(engine, f2, node_set2, kVal0);

  return engine.check_sat() == kB3False;
}

/// @brief f1 と f2 が衝突しているか調べる．
bool
FaultAnalyzer::check_fault_conflict(TpgFault* f1,
				    TpgFault* f2)
{
  GvalCnf gval_cnf2(mMaxNodeId);
  FvalCnf fval_cnf1(mMaxNodeId, gval_cnf2);
  FvalCnf fval_cnf2(mMaxNodeId, gval_cnf2);
  SatEngine engine(string(), string(), NULL);

  NodeSet& node_set1 = mNodeSetArray[f1->node()->id()];
  NodeSet& node_set2 = mNodeSetArray[f2->node()->id()];

  fval_cnf1.make_cnf(engine, f1, node_set1, kVal1);
  fval_cnf2.make_cnf(engine, f2, node_set2, kVal1);

  return engine.check_sat() == kB3False;
}

// @brief analyze_conflict の統計情報を出力する．
void
FaultAnalyzer::print_conflict_stats(ostream& s)
{
  s << "Total    " << setw(6) << mConflictStats.conf_count  << " conflicts" << endl;
  s << "Total    " << setw(6) << mConflictStats.conf1_count << " conflicts (ma_list)" << endl;
  s << "Total    " << setw(6) << mConflictStats.conf3_count << " conflicts (single ma_list)" << endl;
  s << "Total    " << setw(6) << mConflictStats.conf4_count << " conflicts (exact) / "
       << setw(6) << mConflictStats.conf4_check_count << endl;
  s << "Total    " << setw(6) << mConflictStats.int1_count  << " pi_suf_list intersection check" << endl;
  s << "Total    " << setw(6) << mConflictStats.int2_count  << " suf_list intersection check" << endl;
  s << "CPU time (conflict check)    " << mConflictStats.conf_timer.time() << endl;
  s << "CPU time (simple ma_list)    " << mConflictStats.conf1_timer.time() << endl;
  s << "CPU time (single conflict)   " << mConflictStats.conf3_timer.time() << endl;
  s << "CPU time (exact conflict)    " << mConflictStats.conf4_timer.time() << endl;
  s << "CPU time (siple pi_suf_list) " << mConflictStats.int1_timer.time() << endl;
  s << "CPU time (single suf_list)   " << mConflictStats.int2_timer.time() << endl;
}

END_NAMESPACE_YM_SATPG
