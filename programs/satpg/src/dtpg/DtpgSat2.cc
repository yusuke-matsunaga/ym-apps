
/// @file DtpgSat2.cc
/// @brief DtpgSat2 の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2010, 2012-2014 Yusuke Matsunaga
/// All rights reserved.


#include "DtpgSat2.h"
#include "TpgNetwork.h"
#include "TpgNode.h"
#include "TpgFault.h"


BEGIN_NAMESPACE_YM_SATPG

Dtpg*
new_DtpgSat2()
{
  return new DtpgSat2();
}

// @brief コンストラクタ
DtpgSat2::DtpgSat2()
{
}

// @brief デストラクタ
DtpgSat2::~DtpgSat2()
{
}

// @brief 使用する SAT エンジンを指定する．
void
DtpgSat2::set_mode(const string& type,
		   const string& option,
		   ostream* outp)
{
  mSatEngineSingle.set_mode(type, option, outp);
  mSatEngineMulti.set_mode(type, option, outp);
}

// @brief テスト生成を行なう．
// @param[in] tpgnetwork 対象のネットワーク
// @param[in] mode メインモード
// @param[in] po_mode PO分割モード
// @param[in] option_str オプション文字列
// @param[in] bt バックトレーサー
// @param[in] dop_list DetectOp のリスト
// @param[in] uop_list UntestOp のリスト
// @param[in] stats 結果を格納する構造体
void
DtpgSat2::run(TpgNetwork& tgnetwork,
	      DtpgMode mode,
	      tDtpgPoMode po_mode,
	      const string& option_str,
	      BackTracer& bt,
	      DetectOp& dop,
	      UntestOp& uop,
	      DtpgStats& stats)
{
  mSatEngineSingle.set_option(option_str);
  mSatEngineMulti.set_option(option_str);

  tgnetwork.activate_all();

  switch ( mode.mode() ) {
  case kDtpgSingle2:
    single_mode(tgnetwork, bt, dop, uop, stats);
    break;

  case kDtpgMFFC2:
    mffc_mode(tgnetwork, bt, dop, uop, stats);
    break;

  default:
    assert_not_reached(__FILE__, __LINE__);
    break;
  }
}

void
DtpgSat2::single_mode(TpgNetwork& tgnetwork,
		      BackTracer& bt,
		      DetectOp& dop,
		      UntestOp& uop,
		      DtpgStats& stats)
{
  mSatEngineSingle.clear_stats();

  ymuint nn = tgnetwork.node_num();
  for (ymuint i = 0; i < nn; ++ i) {
    TpgNode* node = tgnetwork.node(i);
    if ( node->is_output() ) {
      continue;
    }

    ymuint nf = node->fault_num();
    for (ymuint i = 0; i < nf; ++ i) {
      TpgFault* fault = node->fault(i);
      if ( fault->status() != kFsDetected &&
	   !fault->is_skip() ) {
	mSatEngineSingle.run(tgnetwork, fault, bt, dop, uop);
      }
    }
  }

  mSatEngineSingle.get_stats(stats);
}

void
DtpgSat2::mffc_mode(TpgNetwork& network,
		    BackTracer& bt,
		    DetectOp& dop,
		    UntestOp& uop,
		    DtpgStats& stats)
{
  mSatEngineMulti.clear_stats();

  vector<TpgNode*> node_list;
  ymuint n = network.node_num();
  for (ymuint i = 0; i < n; ++ i) {
    TpgNode* node = network.node(i);
    if ( node->imm_dom() == NULL ) {
      node_list.push_back(node);
    }
  }

  for (ymuint i = 0; i < node_list.size(); ++ i) {
    TpgNode* node = node_list[i];

    clear_faults();

    vector<bool> mark(network.max_node_id(), false);
    dfs_mffc(node, mark);

    do_dtpg(network, bt, dop, uop);
  }

  mSatEngineMulti.get_stats(stats);
}

void
DtpgSat2::dfs_mffc(TpgNode* node,
		   vector<bool>& mark)
{
  mark[node->id()] = true;

  ymuint ni = node->fanin_num();
  for (ymuint i = 0; i < ni; ++ i) {
    TpgNode* inode = node->fanin(i);
    if ( mark[inode->id()] == false && inode->imm_dom() != NULL ) {
      dfs_mffc(inode, mark);
    }
  }

  if ( !node->is_output() ) {
    add_node_faults(node);
  }
}

// @brief ノードの故障を追加する．
void
DtpgSat2::add_node_faults(TpgNode* node)
{
  ymuint nf = node->fault_num();
  for (ymuint i = 0; i < nf; ++ i) {
    TpgFault* fault = node->fault(i);
    if ( fault->status() != kFsDetected &&
	 !fault->is_skip() ) {
      mFaultList.push_back(fault);
    }
  }
}

// @brief テストパタン生成を行なう．
void
DtpgSat2::do_dtpg(TpgNetwork& network,
		  BackTracer& bt,
		  DetectOp& dop,
		  UntestOp& uop)
{
  if ( mFaultList.empty() ) {
    return;
  }

  mSatEngineMulti.run(mFaultList, network.max_node_id(), bt, dop, uop);
}

END_NAMESPACE_YM_SATPG
