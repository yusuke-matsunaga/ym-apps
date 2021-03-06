﻿
/// @file GenPat.cc
/// @brief GenPat の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2012 Yusuke Matsunaga
/// All rights reserved.


#include "GenPat.h"
#include "YmUtils/StopWatch.h"


BEGIN_NAMESPACE_YM

BEGIN_NONAMESPACE

struct Npn4Cannon
{
  ymuint16 mFunc;
  ymuint16 mPerm;
};

// 4入力のNPN同値類代表関数
ymuint32 npn4rep[] = {
#include "npn4rep.h"
};

// 4入力のNPN変換を表す配列
ymuint16 npn4perm[] = {
#include "npn4perm.h"
};

// 4入力のNPN同値類の代表関数への変換を表す配列
Npn4Cannon npn4cannon[] = {
#include "npn4cannon.h"
};

ymuint32 n_compose;
ymuint32 level_over;
ymuint32 duplicate_aig;

// 関数のサポートを求める．
ymuint
support(ymuint16 func)
{
  // 数が少ないので個別にやる．
  ymuint ans = 0U;

  // 0 番めの変数
  ymuint16 c0_0 = func & 0x5555U;
  ymuint16 c0_1 = (func & 0xaaaaU) >> 1;
  if ( c0_0 != c0_1 ) {
    ans |= 1U;
  }

  // 1 番めの変数
  ymuint16 c1_0 = func & 0x3333U;
  ymuint16 c1_1 = (func & 0xccccU) >> 2;
  if ( c1_0 != c1_1 ) {
    ans |= 2U;
  }

  // 2 番めの変数
  ymuint16 c2_0 = func & 0x0f0fU;
  ymuint16 c2_1 = (func & 0xf0f0U) >> 4;
  if ( c2_0 != c2_1 ) {
    ans |= 4U;
  }

  // 3 番めの変数
  ymuint16 c3_0 = func & 0x00ffU;
  ymuint16 c3_1 = (func & 0xff00U) >> 8;
  if ( c3_0 != c3_1 ) {
    ans |= 8;
  }

  return ans;
}

END_NONAMESPACE


//////////////////////////////////////////////////////////////////////
// クラス GenPat
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
GenPat::GenPat()
{
}

// @brief デストラクタ
GenPat::~GenPat()
{
}

// @brief 4入力の全ての関数のパタンを求める．
void
GenPat::operator()(ymuint slack)
{
  StopWatch timer;
  timer.start();

  init_npn4rep();

  mGpList.clear();
  mRepList.clear();
  mCandListArray.clear();
  mFuncArray.clear();
  mFuncLevel.clear();
  mGpHash.clear();

  ymuint np = 1U << 4;
  ymuint nf = 1U << np;

  mFuncArray.resize(nf);
  mFuncLevel.resize(nf, 0xffffffff);

  calc_upper();

  n_compose = 0;
  level_over = 0;
  duplicate_aig = 0;

  mSlack = slack;

  // 定数0と定数1は除く
  mRemainFunc = nf - 2;

  // かなり怪しいコード
  mRemainRep = 221;

  // レベル0のパタンを作る．
  {
    mGpList.push_back(vector<GpHandle>());
    mRepList.push_back(vector<GpHandle>());

    GpHandle ih = mMgr.make_input(0);
    ymuint16 fv = ih.func();
    mFuncLevel[fv] = 0;
    -- mRemainFunc;
    -- mRemainRep;
    mFuncArray[fv].push_back(ih);
    mRepList[0].push_back(ih);
    mGpList[0].push_back(ih);

    for (ymuint i = 1; i < 4; ++ i) {
      GpHandle ih = mMgr.make_input(i);
      ymuint16 fv = ih.func();
      mFuncLevel[fv] = 0;
      -- mRemainFunc;
      mFuncArray[fv].push_back(ih);
      mGpList[0].push_back(ih);
    }
  }

  // レベル1以上のパタンを作る．
  ymuint max_level = 0;
  for (ymuint level1 = 1; ; ++ level1) {
    cout << endl
	 << "level = " << level1 << ", remain_func = " << mRemainFunc << endl
	 << "   remain_rep = " << mRemainRep << endl;
    max_level = level1;

    const vector<GpHandle>& src_list1 = mRepList[level1 - 1];
    ymuint n1 = src_list1.size();
    for (ymuint i1 = 0; i1 < n1; ++ i1) {
      GpHandle handle1 = src_list1[i1];

      mCountHash.clear();
      ymuint level_base = count1(handle1);

      for (ymuint level2 = 0; level2 < level1; ++ level2) {
	const vector<GpHandle>& src_list2 = mGpList[level2];
	ymuint n2 = src_list2.size();
	for (ymuint i2 = 0; i2 < n2; ++ i2) {
	  GpHandle handle2 = src_list2[i2];
	  compose(handle1, handle2, level_base);
	}
      }
    }

    while ( mGpList.size() <= level1 ) {
      mGpList.push_back(vector<GpHandle>());
    }
    while ( mRepList.size() <= level1 ) {
      mRepList.push_back(vector<GpHandle>());
    }

    const vector<GpPair>& gp_and_list = mAndCandPairListArray[level1];
    ymuint n_and = gp_and_list.size();
    for (ymuint i = 0; i < n_and; ++ i) {
      const GpPair& gp = gp_and_list[i];
      GpHandle handle = mMgr.make_and(gp.mHandle1, gp.mHandle2);
      add_pat(handle, level1);
    }

    const vector<GpPair>& gp_xor_list = mXorCandPairListArray[level1];
    ymuint n_xor = gp_xor_list.size();
    for (ymuint i = 0; i < n_xor; ++ i) {
      const GpPair& gp = gp_xor_list[i];
      GpHandle handle = mMgr.make_xor(gp.mHandle1, gp.mHandle2);
      add_pat(handle, level1);
    }
    if ( mRemainFunc == 0 ) {
      cout << "All functions has its patterns" << endl;
      break;
    }
  }
#if 0
  {
    unordered_map<ymuint16, vector<GpHandle> > pat_list;
    vector<ymuint16> flist;
    for (ymuint i = 0; i < n1; ++ i) {
      GpHandle handle = src_list1[i];
      ymuint16 func = handle.func();
      if ( mFuncLevel[func] + mSlack >= level ) {
	if ( pat_list.count(func) == 0 ) {
	  flist.push_back(func);
	}
	pat_list[func].push_back(handle);
      }
    }
    sort(flist.begin(), flist.end());
    for (vector<ymuint16>::iterator p = flist.begin(); p != flist.end(); ++ p) {
      unordered_map<ymuint16, vector<GpHandle> >::iterator q = pat_list.find(*p);
      ASSERT_COND( q != pat_list.end() );
      vector<GpHandle>& handle_list = q->second;
      cout << "Function: " << setw(4) << setfill('0') << hex << *p << dec << endl;
      mMgr.dump_handle(cout, handle_list);
      cout << endl;
    }
  }
#endif

  ymuint total_cost = 0;
  bool error = false;
  for (ymuint i = 1; i < nf - 1; ++ i) {
    if ( mFuncArray[i].empty() ) {
      cerr << "No patterns for function["
	   << setw(4) << setfill('0') << hex << i << dec << "]" << endl;
      error = true;
    }
    else {
      total_cost += mFuncLevel[i];
    }
  }
  if ( error ) {
    cerr << "Error occured" << endl;
  }
  else {
    cout << "# total cost = " << total_cost << endl;

    vector<GpHandle> handle_list;
    for (ymuint i = 1; i < nf - 1; ++ i) {
      if ( mNpnHash.count(i) == 0 ) {
	continue;
      }
      cout << "Function#" << hex << setw(4) << setfill('0')
	   << i << dec << ": "
	   << "Level " << mFuncLevel[i] << ": " << endl;
#if 0
      for (vector<GpHandle>::iterator p = mFuncArray[i].begin();
	   p != mFuncArray[i].end(); ++ p) {
	GpHandle handle = *p;
	cout << " ";
	if ( handle.inv() ) {
	  cout << "~";
	}
	cout << "Node#" << handle.node()->id() << endl;
	handle_list.push_back(handle);
      }
      cout << endl;
#endif
    }
#if 0
    mMgr.dump_handle(cout, handle_list);
#endif
  }

  timer.stop();
  USTime time = timer.time();
  cout << "# # of compose  = " << n_compose << endl
       << "# level over    = " << level_over << endl
       << "# duplicate aig = " << duplicate_aig << endl;
  cout << "# of nodes      = " << mMgr.node_num() << endl
       << "CPU time        = " << time << endl;
}

// @brief 2つのノードから新しいパタンを作る．
// @note 具体的には aig1 & aig2 と ~aig & aig
void
GenPat::compose(GpHandle handle1,
		GpHandle handle2,
		ymuint level_base)
{
  ++ n_compose;

  ymuint32 fv1 = handle1.func();
  ymuint32 fv2 = handle2.func();
  ymuint32 fv3 = fv1 & fv2;
  ymuint32 fv1_n = fv1 ^ 0xFFFF;
  ymuint32 fv4 = fv1_n & fv2;
  ymuint32 fv5 = fv1 ^ fv2;

  ymuint sup0 = support(fv1) | support(fv2);

  ymuint sup3 = support(fv3);
  bool valid1 = true;
  if ( sup3 != sup0 ) {
    valid1 = false;
  }
  else if ( fv3 == fv1 || fv3 == fv2 ) {
    valid1 = false;
  }
  else if ( (mFuncLevel[fv3] + mSlack) < level_base + 1 ) {
    valid1 = false;
  }

  ymuint sup4 = support(fv4);
  bool valid2 = true;
  if ( sup4 != sup0 ) {
    valid2 = false;
  }
  else if ( fv4 == fv1 || fv4 == fv2 ) {
    valid2 = false;
  }
  else if ( (mFuncLevel[fv4] + mSlack) < level_base + 1 ) {
    valid2 = false;
  }

  ymuint sup5 = support(fv5);
  bool valid3 = true;
  if ( sup5 != sup0 ) {
    valid3 = false;
  }
  else if ( fv5 == fv1 || fv5 == fv2 ) {
    valid3 = false;
  }
  else if ( (mFuncLevel[fv5] + mSlack) < level_base + 1 ) {
    valid3 = false;
  }

  if ( !valid1 && !valid2 && !valid3 ) {
    return;
  }

  ymuint level = count2(handle2) + level_base + 1;

  if ( valid1 ) {
    add_cand(handle1, handle2, level, mAndCandPairListArray);
  }

  if ( valid2 ) {
    add_cand(~handle1, handle2, level, mAndCandPairListArray);
  }

  if ( valid3 ) {
    add_cand(handle1, handle2, level, mXorCandPairListArray);
  }
}

// @brief 候補のリストに追加する．
void
GenPat::add_cand(GpHandle handle1,
		 GpHandle handle2,
		 ymuint level,
		 vector<vector<GpPair> >& list_array)
{
  while ( list_array.size() <= level ) {
    list_array.push_back(vector<GpPair>());
  }

  list_array[level].push_back(GpPair(handle1, handle2));
}

// @brief GpHandle を登録する．
void
GenPat::add_pat(GpHandle handle,
		ymuint level)
{
  NpnXform xf(npn4cannon[handle.func()].mPerm);
  GpHandle chandle = xform4(handle, xf);

  GpNode* node = chandle.node();
  if ( mGpHash.count(node->id()) == 0 ) {
    mGpHash.insert(node->id());

    ymuint16 fv = chandle.func();
    if ( mFuncArray[fv].empty() ) {
      mFuncArray[fv].push_back(chandle);
      mFuncLevel[fv] = level;
      -- mRemainFunc;
      -- mRemainRep;
    }
    else if ( mFuncLevel[fv] > level ) {
      // ここにくることはないはず
      mFuncLevel[fv] = level;
    }
    mGpList[level].push_back(chandle);
    mRepList[level].push_back(chandle);

    unordered_map<ymuint32, vector<FuncXform> >::const_iterator p;
    p = mNpnHash.find(chandle.func());
    ASSERT_COND( p != mNpnHash.end() );
    const vector<FuncXform>& xf_list = p->second;
    for (vector<FuncXform>::const_iterator q = xf_list.begin();
	 q != xf_list.end(); ++ q) {
      const FuncXform& xform = *q;
      GpHandle handle1 = xform4(chandle, xform.mXf);
      mGpList[level].push_back(handle1);
#if 1
      ymint16 fv1 = handle1.func();
      if ( fv1 != fv && mFuncArray[fv1].empty() ) {
	mFuncArray[fv1].push_back(handle1);
	mFuncLevel[fv1] = level;
	-- mRemainFunc;
      }
#endif
    }
  }
  else {
    ++ duplicate_aig;
  }
}

// @brief handle の子供に印をつけてノード数を数える．
ymuint
GenPat::count1(GpHandle handle)
{
  GpNode* node = handle.node();
  if ( node == NULL ) {
    return 0;
  }

  if ( mCountHash.count(node->id()) > 0 ) {
    return 0;
  }

  mCountHash.insert(node->id());
  ymuint ans = 0;
  if ( node->is_logic() ) {
    ans = 1;
    ans += count1(node->fanin0());
    ans += count1(node->fanin1());
  }
  return ans;
}

// @brief count1 で印のついていないノード数を数える．
ymuint
GenPat::count2(GpHandle handle)
{
  unordered_set<ymuint32> hash1;
  return count2_sub(handle.node(), hash1);
}

// @brief count2 の下請け関数
ymuint
GenPat::count2_sub(GpNode* node,
		   unordered_set<ymuint32>& hash)
{
  if ( node == NULL ) {
    return 0;
  }

  if ( mCountHash.count(node->id()) > 0 ||
       hash.count(node->id()) > 0 ) {
    return 0;
  }

  hash.insert(node->id());
  ymuint ans = 0;
  if ( node->is_logic() ) {
    ans = 1;
    ans += count2_sub(node->fanin0_node(), hash);
    ans += count2_sub(node->fanin1_node(), hash);
  }
  return ans;
}


BEGIN_NONAMESPACE

// @brief 関数ベクタを変換する(4入力版)
ymuint16
xform_func4(ymuint16 fv,
	    NpnXform xf)
{
  ymuint16 ans = 0U;
  for (ymuint p = 0; p < 16; ++ p) {
    if ( fv & (1U << p) ) {
      ymuint v0 = (p >> 0) & 1U;
      ymuint v1 = (p >> 1) & 1U;
      ymuint v2 = (p >> 2) & 1U;
      ymuint v3 = (p >> 3) & 1U;
      if ( xf.input_inv(0) ) {
	v0 ^= 1U;
      }
      if ( xf.input_inv(1) ) {
	v1 ^= 1U;
      }
      if ( xf.input_inv(2) ) {
	v2 ^= 1U;
      }
      if ( xf.input_inv(3) ) {
	v3 ^= 1U;
      }
      ymuint q = 0U;
      if ( v0 ) {
	q |= (1U << xf.input_perm(0));
      }
      if ( v1 ) {
	q |= (1U << xf.input_perm(1));
      }
      if ( v2 ) {
	q |= (1U << xf.input_perm(2));
      }
      if ( v3 ) {
	q |= (1U << xf.input_perm(3));
      }
      ans |= (1U << q);
    }
  }
  if ( xf.output_inv() ) {
    ans ^= 0xFFFF;
  }
  return ans;
}

END_NONAMESPACE

// @brief 4入力関数の情報をセットアップする．
void
GenPat::init_npn4rep()
{
  for (ymuint i = 0; npn4rep[i] != 0xFFFF; ++ i) {
    ymuint32 fv = npn4rep[i];

    vector<FuncXform> tmp_list;
    unordered_set<ymuint32> func_hash;
    for (ymuint p = 0; p < 768; ++ p) {
      NpnXform xf(npn4perm[p]);
      ymuint16 fv1 = xform_func4(fv, xf);
      if ( func_hash.count(fv1) == 0 ) {
	func_hash.insert(fv1);
	tmp_list.push_back(FuncXform(fv1, xf.data()));
      }
    }

    mNpnHash.insert(make_pair(fv, tmp_list));
  }
}

// @brief GpHandle を変換する(4入力版)
GpHandle
GenPat::xform4(GpHandle handle,
	       NpnXform xf)
{
  if ( handle.is_const1() || handle.is_const0() ) {
    if ( xf.output_inv() ) {
      handle = ~handle;
    }
    return handle;
  }

  GpHandle handle1 = xf4_sub(handle, xf);
  if ( xf.output_inv() ) {
    handle1 = ~handle1;
  }
  return handle1;
}

// @brief xform4 の下請け関数
GpHandle
GenPat::xf4_sub(GpHandle handle,
		NpnXform xf)
{
  GpNode* node = handle.node();
  bool inv = handle.inv();

  if (node->is_input() ) {
    ymuint id = node->input_id();
    ymuint new_id = xf.input_perm(id);
    GpHandle handle1 = mMgr.make_input(new_id);
    if ( inv ) {
      handle1 = ~handle1;
    }
    if ( xf.input_inv(id) ) {
      handle1 = ~handle1;
    }
    return handle1;
  }

  GpHandle handle1 = xf4_sub(node->fanin0(), xf);
  GpHandle handle2 = xf4_sub(node->fanin1(), xf);
  GpHandle ans;
  if ( node->is_and() ) {
    ans = mMgr.make_and(handle1, handle2);
  }
  else {
    ans = mMgr.make_xor(handle1, handle2);
  }
  if ( inv ) {
    ans = ~ans;
  }
  return ans;
}

// @brief 関数ごとのレベルの上限をセットする．
void
GenPat::calc_upper()
{
  ymuint np = 1U << 4;
  ymuint nf = 1U << np;

  mUpperLevel.resize(nf, 0xffff);

  mRemainFunc = nf - 2;

  // レベル0の関数を求める．
  mFuncListArray.push_back(vector<ymuint16>());
  for (ymuint i = 0; i < 4; ++ i) {
    ymuint16 fv;
    switch ( i ) {
    case 0: fv = 0xaaaa; break;
    case 1: fv = 0xcccc; break;
    case 2: fv = 0xf0f0; break;
    case 3: fv = 0xff00; break;
    }
    add_func(fv, 0);
  }

  // レベル1以上の関数を求める．
  ymuint max_level = 0;
  for (ymuint level = 1; mRemainFunc > 0; ++ level) {
    cout << endl
	 << "level = " << level << ", remain func = " << mRemainFunc << endl;
    max_level = level;
    mFuncListArray.push_back(vector<ymuint16>());
    ymuint h = (level + 1) / 2;
    for (ymuint llevel = 0; llevel < h; ++ llevel) {
      ymuint rlevel = level - llevel - 1;
      cout << "  l-level = " << llevel
	   << ", r-level = " << rlevel << endl;
      if ( llevel == rlevel ) {
	const vector<ymuint16>& func_list = mFuncListArray[llevel];
	ymuint n = func_list.size();
	for (ymuint l = 0; l < n; ++ l) {
	  ymuint16 func1 = func_list[l];
	  for (ymuint r = l + 1; r < n; ++ r) {
	    ymuint16 func2 = func_list[r];
	    add_func(func1 & func2, level);
	    add_func(func1 ^ func2, level);
	  }
	}
      }
      else {
	const vector<ymuint16>& l_func_list = mFuncListArray[llevel];
	const vector<ymuint16>& r_func_list = mFuncListArray[rlevel];
	ymuint nl = l_func_list.size();
	ymuint nr = r_func_list.size();
	for (ymuint l = 0; l < nl; ++ l) {
	  ymuint16 func1 = l_func_list[l];
	  for (ymuint r = 0; r < nr; ++ r) {
	    ymuint16 func2 = r_func_list[r];
	    add_func(func1 & func2, level);
	    add_func(func1 ^ func2, level);
	  }
	}
      }
    }
  }

  bool error = false;
  for (ymuint p = 1; p < nf - 1; ++ p) {
    if ( mUpperLevel[p] == 0xFFFF ) {
      cout << "No patterns for function["
	   << setw(4) << setfill('0') << hex << p << dec << "]" << endl;
      error = true;
    }
    else {
      mFuncLevel[p] = mUpperLevel[p];
    }
  }
}

void
GenPat::add_func(ymuint16 func,
		 ymuint level)
{
  if ( func == 0U || func == 0xFFFF ) {
    return;
  }

  if ( mUpperLevel[func] == 0xFFFF ) {
    mUpperLevel[func] = level;
    mFuncListArray[level].push_back(func);
    -- mRemainFunc;
  }
  ymuint16 func_n = func ^ 0xFFFF;
  if ( mUpperLevel[func_n] == 0xFFFF ) {
    mUpperLevel[func_n] = level;
    mFuncListArray[level].push_back(func_n);
    -- mRemainFunc;
  }
}

END_NAMESPACE_YM
