
/// @file FgMgr2.cc
/// @brief FgMgr2 の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2015 Yusuke Matsunaga
/// All rights reserved.


#include "FgMgr2.h"

#include "TpgFault.h"
#include "NodeSet.h"
#include "GvalCnf.h"
#include "FvalCnf.h"
#include "SatEngine.h"
#include "YmUtils/HashSet.h"


BEGIN_NAMESPACE_YM_SATPG

//////////////////////////////////////////////////////////////////////
// クラス FgMgr2
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] max_node_id ノード番号の最大値 + 1
FgMgr2::FgMgr2(ymuint max_node_id) :
  mMaxNodeId(max_node_id)
{
}

// @brief デストラクタ
FgMgr2::~FgMgr2()
{
  clear();
}

// @brief クリアする．
void
FgMgr2::clear()
{
  for (ymuint gid = 0; gid < mGroupList.size(); ++ gid) {
    delete mGroupList[gid];
  }
  mGroupList.clear();
}

// @brief 現在のグループ数を返す．
ymuint
FgMgr2::group_num() const
{
  return mGroupList.size();
}

// @brief 新しいグループを作る．
// @return グループ番号を返す．
ymuint
FgMgr2::new_group()
{
  FaultGroup* fg = new FaultGroup;
  bool found = false;
  for (ymuint i = 0; i < mGroupList.size(); ++ i) {
    if ( mGroupList[i] == NULL ) {
      fg->mId = i;
      mGroupList[i] = fg;
      found = true;
      break;
    }
  }
  if ( !found ) {
    fg->mId = mGroupList.size();
    mGroupList.push_back(fg);
  }
  return fg->mId;
}

// @brief グループを複製する．
// @param[in] src_gid 複製元のグループ番号
// @return 新しいグループ番号を返す．
ymuint
FgMgr2::duplicate_group(ymuint src_gid)
{
  ASSERT_COND( src_gid < mGroupList.size() );
  FaultGroup* src_fg = mGroupList[src_gid];
  ASSERT_COND( src_fg != NULL );
  ymuint gid = new_group();
  FaultGroup* dst_fg = mGroupList[gid];
  dst_fg->mFaultList = src_fg->mFaultList;
  dst_fg->mFaultSufList = src_fg->mFaultSufList;
  dst_fg->mSufList = src_fg->mSufList;
  return gid;
}

// @brief グループを置き換える．
// @param[in] old_gid 置き換え対象のグループ番号
// @param[in] new_gid 置き換えるグループ番号
//
// new_gid は削除される．
void
FgMgr2::replace_group(ymuint old_gid,
		     ymuint new_gid)
{
  delete_group(old_gid);

  ASSERT_COND( new_gid < mGroupList.size() );
  FaultGroup* new_fg = mGroupList[new_gid];
  ASSERT_COND( new_fg != NULL );
  new_fg->mId = old_gid;
  mGroupList[old_gid] = new_fg;
  mGroupList[new_gid] = NULL;
}

// @brief グループを削除する．
// @param[in] gid グループ番号
void
FgMgr2::delete_group(ymuint gid)
{
  ASSERT_COND( gid < mGroupList.size() );
  FaultGroup* fg = mGroupList[gid];
  ASSERT_COND( fg != NULL );
  delete fg;
  mGroupList[gid] = NULL;
}

// @brief 新たな条件なしで追加できる既存グループを見つける．
// @param[in] fault 対象の故障
// @param[in] group_list 探索最小のグループ番号のリスト
// @param[in] first_hit 最初のグループのみを求めるとき true にするフラグ
// @param[out] gid_list 対象のグループ番号を収めるリスト
// @return 最初のグループ番号を返す．
//
// 見つからない場合は group_num() を返す．
// gid_list は first_hit == true の時，意味を持たない．
ymuint
FgMgr2::find_dom_group(const TpgFault* fault,
		       const vector<ymuint>& group_list,
		       bool first_hit,
		       vector<ymuint>& gid_list)
{
  ymuint first_gid = group_num();
  for (ymuint i = 0; i < group_list.size(); ++ i) {
    ymuint gid = group_list[i];

    GvalCnf gval_cnf(mMaxNodeId);
    FvalCnf fval_cnf(mMaxNodeId, gval_cnf);
    SatEngine engine(string(), string(), NULL);

    NodeSet node_set;
    node_set.mark_region(mMaxNodeId, fault->node());

    engine.make_fval_cnf(fval_cnf, fault, node_set, kVal0);

    const vector<const TpgFault*>& flist = fault_list(gid);
    ymuint nf = flist.size();
    vector<FvalCnf*> fval_cnf_array(nf);
    vector<NodeSet> node_set_array(nf);
    for (ymuint i = 0; i < nf; ++ i) {
      const TpgFault* fault = flist[i];
      FvalCnf* fval_cnfp = new FvalCnf(mMaxNodeId, gval_cnf);
      fval_cnf_array[i] = fval_cnfp;
      node_set_array[i].mark_region(mMaxNodeId, fault->node());

      engine.make_fval_cnf(*fval_cnfp, fault, node_set_array[i], kVal1);
    }
    Bool3 sat_ans = engine.check_sat();
    for (ymuint i = 0; i < nf; ++ i) {
      delete fval_cnf_array[i];
    }
    if ( sat_ans == kB3False ) {
      if ( first_gid == group_num() ) {
	first_gid = gid;
	if ( first_hit ) {
	  break;
	}
      }
      gid_list.push_back(gid);
    }
  }
  return first_gid;
}

// @brief 追加できる既存グループを見つける．
// @param[in] fault 対象の故障
// @param[in] group_list 探索最小のグループ番号のリスト
// @param[in] first_hit 最初のグループのみを求めるとき true にするフラグ
// @param[out] gid_list 対象のグループ番号を収めるリスト
// @return 最初のグループ番号を返す．
//
// 見つからない場合は group_num() を返す．
// gid_list は first_hit == true の時，意味を持たない．
ymuint
FgMgr2::find_group(const TpgFault* fault,
		   const vector<ymuint>& group_list,
		   bool first_hit,
		   vector<ymuint>& gid_list)
{
  ymuint first_gid = group_num();
  for (ymuint i = 0; i < group_list.size(); ++ i) {
    ymuint gid = group_list[i];

    GvalCnf gval_cnf(mMaxNodeId);
    FvalCnf fval_cnf(mMaxNodeId, gval_cnf);
    SatEngine engine(string(), string(), NULL);

    NodeSet node_set;
    node_set.mark_region(mMaxNodeId, fault->node());

    engine.make_fval_cnf(fval_cnf, fault, node_set, kVal1);

    const vector<const TpgFault*>& flist = fault_list(gid);
    ymuint nf = flist.size();
    vector<FvalCnf*> fval_cnf_array(nf);
    vector<NodeSet> node_set_array(nf);
    for (ymuint i = 0; i < nf; ++ i) {
      const TpgFault* fault = flist[i];
      FvalCnf* fval_cnfp = new FvalCnf(mMaxNodeId, gval_cnf);
      fval_cnf_array[i] = fval_cnfp;
      node_set_array[i].mark_region(mMaxNodeId, fault->node());

      engine.make_fval_cnf(*fval_cnfp, fault, node_set_array[i], kVal1);
    }
    Bool3 sat_ans = engine.check_sat();
    for (ymuint i = 0; i < nf; ++ i) {
      delete fval_cnf_array[i];
    }
    if ( sat_ans == kB3True ) {
      if ( first_gid == group_num() ) {
	first_gid = gid;
	if ( first_hit ) {
	  break;
	}
      }
      gid_list.push_back(gid);
    }
  }
  return first_gid;
}

// @brief 既存のグループに故障を追加する．
// @param[in] gid グループ番号 ( 0 <= gid < group_num() )
// @param[in] fault 故障
void
FgMgr2::add_fault(ymuint gid,
		 const TpgFault* fault)
{
  ASSERT_COND( gid < group_num() );
  FaultGroup* fg = mGroupList[gid];

  GvalCnf gval_cnf(mMaxNodeId);
  FvalCnf fval_cnf(mMaxNodeId, gval_cnf);
  SatEngine engine(string(), string(), NULL);

  NodeSet node_set;
  node_set.mark_region(mMaxNodeId, fault->node());

  engine.make_fval_cnf(fval_cnf, fault, node_set, kVal1);

  ymuint nf = fg->mFaultList.size();
  vector<FvalCnf*> fval_cnf_array(nf);
  vector<NodeSet> node_set_array(nf);
  for (ymuint i = 0; i < nf; ++ i) {
    const TpgFault* fault = fg->mFaultList[i];
    FvalCnf* fval_cnfp = new FvalCnf(mMaxNodeId, gval_cnf);
    fval_cnf_array[i] = fval_cnfp;
    node_set_array[i].mark_region(mMaxNodeId, fault->node());

    engine.make_fval_cnf(*fval_cnfp, fault, node_set_array[i], kVal1);
  }
  vector<Bool3> sat_model;
  Bool3 sat_ans = engine.check_sat(sat_model);
  ASSERT_COND( sat_ans == kB3True );
  fg->mSufList.clear();
  fg->mPiSufList.clear();
  for (ymuint i = 0; i < nf; ++ i) {
    const TpgFault* fault = fg->mFaultList[i];
    const NodeSet& node_set = node_set_array[i];
    NodeValList suf_list;
    NodeValList pi_suf_list;
    fval_cnf_array[i]->get_pi_suf_list(sat_model, fault, node_set, suf_list, pi_suf_list);
    fg->mFaultSufList[i] = suf_list;
    fg->mFaultPiSufList[i] = pi_suf_list;
    fg->mSufList.merge(suf_list);
    fg->mPiSufList.merge(pi_suf_list);
    delete fval_cnf_array[i];
  }

  NodeValList suf_list;
  NodeValList pi_suf_list;
  fval_cnf.get_pi_suf_list(sat_model, fault, node_set, suf_list, pi_suf_list);

  fg->add_fault(fault, suf_list, pi_suf_list);
}

// @brief 故障を取り除く
// @param[in] gid グループ番号 ( 0 <= gid < group_num() )
// @param[in] fault_list 故障リスト
void
FgMgr2::delete_fault(ymuint gid,
		    const vector<const TpgFault*>& fault_list)
{
  ASSERT_COND( gid < group_num() );
  FaultGroup* fg = mGroupList[gid];

  HashSet<ymuint> fault_hash;
  for (ymuint i = 0; i < fault_list.size(); ++ i) {
    const TpgFault* fault = fault_list[i];
    fault_hash.add(fault->id());
  }

  fg->mSufList = NodeValList();
  ymuint nf = fg->mFaultList.size();
  ymuint wpos = 0;
  for (ymuint i = 0; i < nf; ++ i) {
    const TpgFault* fault = fg->mFaultList[i];
    if ( fault_hash.check(fault->id()) ) {
      continue;
    }
    if ( wpos != i ) {
      fg->mFaultList[wpos] = fault;
      fg->mFaultSufList[wpos] = fg->mFaultSufList[i];
    }
    ++ wpos;
    fg->mSufList.merge(fg->mFaultSufList[i]);
  }
  fg->mFaultList.erase(fg->mFaultList.begin() + wpos, fg->mFaultList.end());
  fg->mFaultSufList.erase(fg->mFaultSufList.begin() + wpos, fg->mFaultSufList.end());
}

// @brief 故障リストを返す．
// @param[in] gid グループ番号 ( 0 <= gid < group_num() )
const vector<const TpgFault*>&
FgMgr2::fault_list(ymuint gid) const
{
  ASSERT_COND( gid < group_num() );
  FaultGroup* fg = mGroupList[gid];
  return fg->mFaultList;
}

// @brief 十分割当リストを返す．
// @param[in] gid グループ番号 ( 0 <= gid < group_num() )
const NodeValList&
FgMgr2::sufficient_assignment(ymuint gid) const
{
  ASSERT_COND( gid < group_num() );
  FaultGroup* fg = mGroupList[gid];
  return fg->mSufList;
}

// @brief 外部入力上の十分割当リストを返す．
// @param[in] gid グループ番号 ( 0 <= gid < group_num() )
const NodeValList&
FgMgr2::pi_sufficient_assignment(ymuint gid) const
{
  ASSERT_COND( gid < group_num() );
  FaultGroup* fg = mGroupList[gid];
  return fg->mPiSufList;
}

END_NAMESPACE_YM_SATPG
