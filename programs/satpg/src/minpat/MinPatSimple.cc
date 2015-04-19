﻿
/// @file MinPatSimple.cc
/// @brief MinPatSimple の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2011, 2013-2014, 2015 Yusuke Matsunaga
/// All rights reserved.


#include "MinPatSimple.h"
#include "TpgFault.h"
#include "FaultAnalyzer.h"

#include "YmUtils/RandGen.h"


BEGIN_NAMESPACE_YM_SATPG

// @brief インスタンスを生成する関数
// @param[in] group_dominance グループ支配を計算する．
MinPat*
new_MinPatSimple(bool group_dominance)
{
  return new MinPatSimple(group_dominance);
}

BEGIN_NONAMESPACE

struct FaultLt2
{
  FaultLt2(const vector<FaultInfo>& fault_info_array) :
    mFaultInfoArray(fault_info_array)
  {
  }

  bool
  operator()(TpgFault* left,
	     TpgFault* right)
  {
    return mFaultInfoArray[left->id()].detnum() < mFaultInfoArray[right->id()].detnum();
  }

  const vector<FaultInfo>& mFaultInfoArray;

};

END_NONAMESPACE

//////////////////////////////////////////////////////////////////////
// クラス MinPatSimple
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] group_dominance グループ支配を計算する．
MinPatSimple::MinPatSimple(bool group_dominance) :
  MinPatNaive(group_dominance)
{
}

// @brief デストラクタ
MinPatSimple::~MinPatSimple()
{
}

// @brief 初期化を行う．
// @param[in] network 対象のネットワーク
// @param[in] tvmgr テストベクタマネージャ
// @param[in] fsim2 2値の故障シミュレータ(検証用)
// @param[out] fault_list 検出された故障のリスト
void
MinPatSimple::init(TpgNetwork& network,
		   TvMgr& tvmgr,
		   Fsim& fsim2,
		   vector<TpgFault*>& fault_list)
{
  FaultAnalyzer analyzer;

  analyzer.set_verbose(verbose());

  analyzer.init(network, tvmgr);

  fault_list = analyzer.fault_list();

  RandGen rg;
  analyzer.get_pat_list(fsim2, tvmgr, rg);

  analyzer.get_dom_faults(dom_method());

  const vector<TpgFault*>& src_list = analyzer.dom_fault_list();
  ymuint nf = src_list.size();

  // 故障を検出パタン数の少ない順に並べる．
  vector<TpgFault*> tmp_list = src_list;
  sort(tmp_list.begin(), tmp_list.end(), FaultLt2(analyzer.fault_info_array()));
  set_fault_list(tmp_list);
}

END_NAMESPACE_YM_SATPG
