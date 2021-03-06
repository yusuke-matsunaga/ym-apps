﻿
/// @file calc_cvf/SnAnd.cc
/// @brief SnAnd の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
/// 
/// $Id: SnAnd.cc 1978 2009-02-06 12:29:16Z matsunaga $
///
/// Copyright (C) 2005-2008 Yusuke Matsunaga
/// All rights reserved.

#if HAVE_CONFIG_H
#include "seal_config.h"
#endif


#include "SnAnd.h"


BEGIN_NAMESPACE_YM_SEAL_CVF

//////////////////////////////////////////////////////////////////////
// SnAnd
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
SnAnd::SnAnd(ymuint32 id,
	     const vector<SimNode*>& inputs) :
  SnGate(id, inputs)
{
}

// @brief デストラクタ
SnAnd::~SnAnd()
{
}

// @brief 正常値の計算を行う．
tPackedVal
SnAnd::_calc_gval()
{
  size_t n = mNfi;
  tPackedVal new_val = mFanins[0]->get_gval();
  for (size_t i = 1; i < n; ++ i) {
    new_val &= mFanins[i]->get_gval();
  }
  return new_val;
}

// @brief 故障値の計算を行う．
tPackedVal
SnAnd::_calc_fval()
{
  size_t n = mNfi;
  tPackedVal new_val = mFanins[0]->get_fval();
  for (size_t i = 1; i < n; ++ i) {
    new_val &= mFanins[i]->get_fval();
  }
  return new_val;
}

// @brief 入力の擬似最小 obs を計算する．
void
SnAnd::calc_pseudo_min_iobs()
{
  size_t ni = mNfi;
  tPackedVal tmp0 = kPvAll1;
  for (size_t i = 1; i <= ni; ++ i) {
    size_t j = ni - i;
    mTmp[j] = tmp0;
    tmp0 &= mFanins[j]->get_gval();
  }
  tmp0 = get_obs();
  for (size_t i = 0; i < ni; ++ i) {
    SimNode* inode = mFanins[i];
    inode->or_obs(tmp0 & mTmp[i]);
    tmp0 &= inode->get_gval();
  }
}

// @brief 入力の最大 obs を計算する．
void
SnAnd::calc_max_iobs(RandGen& randgen)
{
  random_order(randgen);
  
  tPackedVal tmp0 = kPvAll1;
  for (size_t i = 1; i <= mNfi; ++ i) {
    size_t j = mNfi - i;
    mTmp[j] = tmp0;
    tmp0 &= mFanins[mOrder[j]]->get_gval();
  }
  tmp0 = get_obs2();
  for (size_t i = 0; i < mNfi; ++ i) {
    SimNode* inode = mFanins[mOrder[i]];
    tPackedVal val1 = inode->get_gval();
    inode->or_obs2(tmp0 & (mTmp[i] | ~val1));
    tmp0 &= val1;
  }
}

// @brief 入力の obs を計算する．
void
SnAnd::_calc_iobs(tPackedVal obs)
{
  size_t ni = mNfi;
  tPackedVal tmp0 = kPvAll1;
  for (size_t i = 1; i <= ni; ++ i) {
    size_t j = ni - i;
    mTmp[j] = tmp0;
    tmp0 &= mFanins[j]->get_gval();
  }
  tmp0 = obs;
  for (size_t i = 0; i < ni; ++ i) {
    SimNode* inode = mFanins[i];
    inode->calc_iobs(tmp0 & mTmp[i]);
    tmp0 &= inode->get_gval();
  }
}

// @brief 正常値の計算を行う．
Bdd
SnAnd::_calc_gfunc()
{
  size_t n = mNfi;
  Bdd new_func = mFanins[0]->get_gfunc();
  for (size_t i = 1; i < n; ++ i) {
    new_func &= mFanins[i]->get_gfunc();
  }
  return new_func;
}

// @brief 故障値の計算を行う．
Bdd
SnAnd::_calc_ffunc()
{
  size_t n = mNfi;
  Bdd new_func = mFanins[0]->get_ffunc();
  for (size_t i = 1; i < n; ++ i) {
    new_func &= mFanins[i]->get_ffunc();
  }
  return new_func;
}

// @brief 入力の擬似最小 obs を計算する．
void
SnAnd::calc_pseudo_min_iobsfunc()
{
  size_t ni = mNfi;
  Bdd tmp0 = mFanins[0]->get_gfunc().mgr().make_one();
  for (size_t i = 1; i <= ni; ++ i) {
    size_t j = ni - i;
    mTmpFunc[j] = tmp0;
    tmp0 &= mFanins[j]->get_gfunc();
  }
  tmp0 = get_obsfunc();
  for (size_t i = 0; i < ni; ++ i) {
    SimNode* inode = mFanins[i];
    inode->or_obsfunc(tmp0 & mTmpFunc[i]);
    tmp0 &= inode->get_gfunc();
  }
}
  
// @brief 入力の最大 obs を計算する．
void
SnAnd::calc_max_iobsfunc()
{
  size_t ni = mNfi;
  Bdd tmp0 = mFanins[0]->get_gfunc().mgr().make_one();
  for (size_t i = 1; i <= ni; ++ i) {
    size_t j = ni - i;
    mTmpFunc[j] = tmp0;
    tmp0 &= mFanins[j]->get_gfunc();
  }
  tmp0 = get_obsfunc();
  for (size_t i = 0; i < ni; ++ i) {
    SimNode* inode = mFanins[i];
    Bdd val1 = inode->get_gfunc();
    inode->or_obsfunc(tmp0 & (mTmpFunc[i] | ~val1));
    tmp0 &= val1;
  }
}
  
// @brief 入力の obs を計算する．
void
SnAnd::_calc_iobsfunc(const Bdd& obs)
{
  size_t ni = mNfi;
  Bdd tmp0 = mFanins[0]->get_gfunc().mgr().make_one();
  for (size_t i = 1; i <= ni; ++ i) {
    size_t j = ni - i;
    mTmpFunc[j] = tmp0;
    tmp0 &= mFanins[j]->get_gfunc();
  }
  tmp0 = obs;
  for (size_t i = 0; i < ni; ++ i) {
    SimNode* inode = mFanins[i];
    inode->calc_iobsfunc(tmp0 & mTmpFunc[i]);
    tmp0 &= inode->get_gfunc();
  }
}

// @brief 内容をダンプする．
void
SnAnd::dump(ostream& s) const
{
  size_t n = mNfi;
  s << "AND(" << mFanins[0]->id();
  for (size_t i = 1; i < n; ++ i) {
    s << ", " << mFanins[i]->id();
  }
  s << ")" << endl;
}


//////////////////////////////////////////////////////////////////////
// SnAnd2
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
SnAnd2::SnAnd2(ymuint32 id,
	       const vector<SimNode*>& inputs) :
  SnGate2(id, inputs)
{
}

// @brief デストラクタ
SnAnd2::~SnAnd2()
{
}

// @brief 正常値の計算を行う．
tPackedVal
SnAnd2::_calc_gval()
{
  tPackedVal pat0 = mFanins[0]->get_gval();
  tPackedVal pat1 = mFanins[1]->get_gval();
  return pat0 & pat1;
}

// @brief 故障値の計算を行う．
tPackedVal
SnAnd2::_calc_fval()
{
  tPackedVal pat0 = mFanins[0]->get_fval();
  tPackedVal pat1 = mFanins[1]->get_fval();
  return pat0 & pat1;
}

// @brief 入力の擬似最小 obs を計算する．
void
SnAnd2::calc_pseudo_min_iobs()
{
  tPackedVal obs = get_obs();
  mFanins[0]->or_obs(obs & mFanins[1]->get_gval());
  mFanins[1]->or_obs(obs & mFanins[0]->get_gval());
}

// @brief 入力の最大 obs を計算する．
void
SnAnd2::calc_max_iobs(RandGen& randgen)
{
  random_order(randgen);
  
  tPackedVal obs = get_obs2();
  tPackedVal v0 = mFanins[mOrder[0]]->get_gval();
  tPackedVal v1 = mFanins[mOrder[1]]->get_gval();
  mFanins[mOrder[0]]->or_obs2(obs & (v1 | ~v0));
  mFanins[mOrder[1]]->or_obs2(obs & v0);
}

// @brief 入力の obs を計算する．
void
SnAnd2::_calc_iobs(tPackedVal obs)
{
  mFanins[0]->calc_iobs(obs & mFanins[1]->get_gval());
  mFanins[1]->calc_iobs(obs & mFanins[0]->get_gval());
}

// @brief 正常値の計算を行う．
Bdd
SnAnd2::_calc_gfunc()
{
  Bdd pat0 = mFanins[0]->get_gfunc();
  Bdd pat1 = mFanins[1]->get_gfunc();
  return pat0 & pat1;
}

// @brief 故障値の計算を行う．
Bdd
SnAnd2::_calc_ffunc()
{
  Bdd pat0 = mFanins[0]->get_ffunc();
  Bdd pat1 = mFanins[1]->get_ffunc();
  return pat0 & pat1;
}

// @brief 入力の擬似最小 obs を計算する．
void
SnAnd2::calc_pseudo_min_iobsfunc()
{
  Bdd obs = get_obsfunc();
  mFanins[0]->or_obsfunc(obs & mFanins[1]->get_gfunc());
  mFanins[1]->or_obsfunc(obs & mFanins[0]->get_gfunc());
}
  
// @brief 入力の最大 obs を計算する．
void
SnAnd2::calc_max_iobsfunc()
{
  Bdd obs = get_obsfunc();
  Bdd v0 = mFanins[0]->get_gfunc();
  Bdd v1 = mFanins[1]->get_gfunc();
  mFanins[0]->or_obsfunc(obs & (v1 | ~v0));
  mFanins[1]->or_obsfunc(obs & v0);
}
  
// @brief 入力の obs を計算する．
void
SnAnd2::_calc_iobsfunc(const Bdd& obs)
{
  mFanins[0]->calc_iobsfunc(obs & mFanins[1]->get_gfunc());
  mFanins[1]->calc_iobsfunc(obs & mFanins[0]->get_gfunc());
}

// @brief 内容をダンプする．
void
SnAnd2::dump(ostream& s) const
{
  s << "AND2(" << mFanins[0]->id();
  s << ", " << mFanins[1]->id();
  s << ")" << endl;
}


//////////////////////////////////////////////////////////////////////
// SnAnd3
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
SnAnd3::SnAnd3(ymuint32 id,
	       const vector<SimNode*>& inputs) :
  SnGate3(id, inputs)
{
}

// @brief デストラクタ
SnAnd3::~SnAnd3()
{
}

// @brief 正常値の計算を行う．
tPackedVal
SnAnd3::_calc_gval()
{
  tPackedVal pat0 = mFanins[0]->get_gval();
  tPackedVal pat1 = mFanins[1]->get_gval();
  tPackedVal pat2 = mFanins[2]->get_gval();
  return pat0 & pat1 & pat2;
}

// @brief 故障値の計算を行う．
tPackedVal
SnAnd3::_calc_fval()
{
  tPackedVal pat0 = mFanins[0]->get_fval();
  tPackedVal pat1 = mFanins[1]->get_fval();
  tPackedVal pat2 = mFanins[2]->get_fval();
  return pat0 & pat1 & pat2;
}

// @brief 入力の擬似最小 obs を計算する．
void
SnAnd3::calc_pseudo_min_iobs()
{
  tPackedVal obs = get_obs();
  tPackedVal v0 = mFanins[0]->get_gval() & obs;
  tPackedVal v1 = mFanins[1]->get_gval() & obs;
  tPackedVal v2 = mFanins[2]->get_gval();
  mFanins[0]->or_obs(v1 & v2);
  mFanins[1]->or_obs(v0 & v2);
  mFanins[2]->or_obs(v0 & v1);
}

// @brief 入力の最大 obs を計算する．
void
SnAnd3::calc_max_iobs(RandGen& randgen)
{
  random_order(randgen);
  
  tPackedVal obs = get_obs2();
  tPackedVal v0 = mFanins[mOrder[0]]->get_gval();
  tPackedVal v1 = mFanins[mOrder[1]]->get_gval();
  tPackedVal v2 = mFanins[mOrder[2]]->get_gval();
  tPackedVal l0 = obs;
  tPackedVal l1 = l0 & v0;
  tPackedVal l2 = l1 & v1;
  tPackedVal u1 = v2;
  tPackedVal u0 = u1 & v1;
  mFanins[mOrder[0]]->or_obs2(l0 & (u0 | ~v0));
  mFanins[mOrder[1]]->or_obs2(l1 & (u1 | ~v1));
  mFanins[mOrder[2]]->or_obs2(l2);
}

// @brief 入力の obs を計算する．
void
SnAnd3::_calc_iobs(tPackedVal obs)
{
  tPackedVal v0 = mFanins[0]->get_gval() & obs;
  tPackedVal v1 = mFanins[1]->get_gval() & obs;
  tPackedVal v2 = mFanins[2]->get_gval();
  mFanins[0]->calc_iobs(v1 & v2);
  mFanins[1]->calc_iobs(v0 & v2);
  mFanins[2]->calc_iobs(v0 & v1);
}

// @brief 正常値の計算を行う．
Bdd
SnAnd3::_calc_gfunc()
{
  Bdd pat0 = mFanins[0]->get_gfunc();
  Bdd pat1 = mFanins[1]->get_gfunc();
  Bdd pat2 = mFanins[2]->get_gfunc();
  return pat0 & pat1 & pat2;
}

// @brief 故障値の計算を行う．
Bdd
SnAnd3::_calc_ffunc()
{
  Bdd pat0 = mFanins[0]->get_ffunc();
  Bdd pat1 = mFanins[1]->get_ffunc();
  Bdd pat2 = mFanins[2]->get_ffunc();
  return pat0 & pat1 & pat2;
}

// @brief 入力の擬似最小 obs を計算する．
void
SnAnd3::calc_pseudo_min_iobsfunc()
{
  Bdd obs = get_obsfunc();
  Bdd v0 = mFanins[0]->get_gfunc() & obs;
  Bdd v1 = mFanins[1]->get_gfunc() & obs;
  Bdd v2 = mFanins[2]->get_gfunc();
  mFanins[0]->or_obsfunc(v1 & v2);
  mFanins[1]->or_obsfunc(v0 & v2);
  mFanins[2]->or_obsfunc(v0 & v1);
}
  
// @brief 入力の最大 obs を計算する．
void
SnAnd3::calc_max_iobsfunc()
{
  Bdd obs = get_obsfunc();
  Bdd v0 = mFanins[0]->get_gfunc();
  Bdd v1 = mFanins[1]->get_gfunc();
  Bdd v2 = mFanins[2]->get_gfunc();
  Bdd l0 = obs;
  Bdd l1 = l0 & v0;
  Bdd l2 = l1 & v1;
  Bdd u1 = v2;
  Bdd u0 = u1 & v1;
  mFanins[0]->or_obsfunc(l0 & (u0 | ~v0));
  mFanins[1]->or_obsfunc(l1 & (u1 | ~v1));
  mFanins[2]->or_obsfunc(l2);
}
  
// @brief 入力の obs を計算する．
void
SnAnd3::_calc_iobsfunc(const Bdd& obs)
{
  Bdd v0 = mFanins[0]->get_gfunc() & obs;
  Bdd v1 = mFanins[1]->get_gfunc() & obs;
  Bdd v2 = mFanins[2]->get_gfunc();
  mFanins[0]->calc_iobsfunc(v1 & v2);
  mFanins[1]->calc_iobsfunc(v0 & v2);
  mFanins[2]->calc_iobsfunc(v0 & v1);
}

// @brief 内容をダンプする．
void
SnAnd3::dump(ostream& s) const
{
  s << "AND3(" << mFanins[0]->id();
  s << ", " << mFanins[1]->id();
  s << ", " << mFanins[2]->id();
  s << ")" << endl;
}


//////////////////////////////////////////////////////////////////////
// SnAnd4
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
SnAnd4::SnAnd4(ymuint32 id,
	       const vector<SimNode*>& inputs) :
  SnGate4(id, inputs)
{
}

// @brief デストラクタ
SnAnd4::~SnAnd4()
{
}

// @brief 正常値の計算を行う．
tPackedVal
SnAnd4::_calc_gval()
{
  tPackedVal pat0 = mFanins[0]->get_gval();
  tPackedVal pat1 = mFanins[1]->get_gval();
  tPackedVal pat2 = mFanins[2]->get_gval();
  tPackedVal pat3 = mFanins[3]->get_gval();
  return pat0 & pat1 & pat2 & pat3;
}

// @brief 故障値の計算を行う．
tPackedVal
SnAnd4::_calc_fval()
{
  tPackedVal pat0 = mFanins[0]->get_fval();
  tPackedVal pat1 = mFanins[1]->get_fval();
  tPackedVal pat2 = mFanins[2]->get_fval();
  tPackedVal pat3 = mFanins[3]->get_fval();
  return pat0 & pat1 & pat2 & pat3;
}

// @brief 入力の擬似最小 obs を計算する．
void
SnAnd4::calc_pseudo_min_iobs()
{
  tPackedVal obs = get_obs();
  tPackedVal v0 = mFanins[0]->get_gval();
  tPackedVal v1 = mFanins[1]->get_gval();
  tPackedVal v2 = mFanins[2]->get_gval();
  tPackedVal v3 = mFanins[3]->get_gval();
  tPackedVal l0 = obs;
  tPackedVal l1 = l0 & v0;
  tPackedVal l2 = l1 & v1;
  tPackedVal l3 = l2 & v2;
  tPackedVal u2 = v3;
  tPackedVal u1 = u2 & v2;
  tPackedVal u0 = u1 & v1;
  mFanins[0]->or_obs(l0 & u0);
  mFanins[1]->or_obs(l1 & u1);
  mFanins[2]->or_obs(l2 & u2);
  mFanins[3]->or_obs(l3);
}

// @brief 入力の最大 obs を計算する．
void
SnAnd4::calc_max_iobs(RandGen& randgen)
{
  random_order(randgen);
  
  tPackedVal obs = get_obs2();
  tPackedVal v0 = mFanins[mOrder[0]]->get_gval();
  tPackedVal v1 = mFanins[mOrder[1]]->get_gval();
  tPackedVal v2 = mFanins[mOrder[2]]->get_gval();
  tPackedVal v3 = mFanins[mOrder[3]]->get_gval();
  tPackedVal l0 = obs;
  tPackedVal l1 = l0 & v0;
  tPackedVal l2 = l1 & v1;
  tPackedVal l3 = l2 & v2;
  tPackedVal u2 = v3;
  tPackedVal u1 = u2 & v2;
  tPackedVal u0 = u1 & v1;
  mFanins[mOrder[0]]->or_obs2(l0 & (u0 | ~v0));
  mFanins[mOrder[1]]->or_obs2(l1 & (u1 | ~v1));
  mFanins[mOrder[2]]->or_obs2(l2 & (u2 | ~v2));
  mFanins[mOrder[3]]->or_obs2(l3);
}

// @brief 入力の obs を計算する．
void
SnAnd4::_calc_iobs(tPackedVal obs)
{
  tPackedVal v0 = mFanins[0]->get_gval();
  tPackedVal v1 = mFanins[1]->get_gval();
  tPackedVal v2 = mFanins[2]->get_gval();
  tPackedVal v3 = mFanins[3]->get_gval();
  tPackedVal l0 = obs;
  tPackedVal l1 = l0 & v0;
  tPackedVal l2 = l1 & v1;
  tPackedVal l3 = l2 & v2;
  tPackedVal u2 = v3;
  tPackedVal u1 = u2 & v2;
  tPackedVal u0 = u1 & v1;
  mFanins[0]->calc_iobs(l0 & u0);
  mFanins[1]->calc_iobs(l1 & u1);
  mFanins[2]->calc_iobs(l2 & u2);
  mFanins[3]->calc_iobs(l3);
}

// @brief 正常値の計算を行う．
Bdd
SnAnd4::_calc_gfunc()
{
  Bdd pat0 = mFanins[0]->get_gfunc();
  Bdd pat1 = mFanins[1]->get_gfunc();
  Bdd pat2 = mFanins[2]->get_gfunc();
  Bdd pat3 = mFanins[3]->get_gfunc();
  return pat0 & pat1 & pat2 & pat3;
}

// @brief 故障値の計算を行う．
Bdd
SnAnd4::_calc_ffunc()
{
  Bdd pat0 = mFanins[0]->get_ffunc();
  Bdd pat1 = mFanins[1]->get_ffunc();
  Bdd pat2 = mFanins[2]->get_ffunc();
  Bdd pat3 = mFanins[3]->get_ffunc();
  return pat0 & pat1 & pat2 & pat3;
}

// @brief 入力の擬似最小 obs を計算する．
void
SnAnd4::calc_pseudo_min_iobsfunc()
{
  Bdd obs = get_obsfunc();
  Bdd v0 = mFanins[0]->get_gfunc();
  Bdd v1 = mFanins[1]->get_gfunc();
  Bdd v2 = mFanins[2]->get_gfunc();
  Bdd v3 = mFanins[3]->get_gfunc();
  Bdd l0 = obs;
  Bdd l1 = l0 & v0;
  Bdd l2 = l1 & v1;
  Bdd l3 = l2 & v2;
  Bdd u2 = v3;
  Bdd u1 = u2 & v2;
  Bdd u0 = u1 & v1;
  mFanins[0]->or_obsfunc(l0 & u0);
  mFanins[1]->or_obsfunc(l1 & u1);
  mFanins[2]->or_obsfunc(l2 & u2);
  mFanins[3]->or_obsfunc(l3);
}
  
// @brief 入力の最大 obs を計算する．
void
SnAnd4::calc_max_iobsfunc()
{
  Bdd obs = get_obsfunc();
  Bdd v0 = mFanins[0]->get_gfunc();
  Bdd v1 = mFanins[1]->get_gfunc();
  Bdd v2 = mFanins[2]->get_gfunc();
  Bdd v3 = mFanins[3]->get_gfunc();
  Bdd l0 = obs;
  Bdd l1 = l0 & v0;
  Bdd l2 = l1 & v1;
  Bdd l3 = l2 & v2;
  Bdd u2 = v3;
  Bdd u1 = u2 & v2;
  Bdd u0 = u1 & v1;
  mFanins[0]->or_obsfunc(l0 & (u0 | ~v0));
  mFanins[1]->or_obsfunc(l1 & (u1 | ~v1));
  mFanins[2]->or_obsfunc(l2 & (u2 | ~v2));
  mFanins[3]->or_obsfunc(l3);
}
  
// @brief 入力の obs を計算する．
void
SnAnd4::_calc_iobsfunc(const Bdd& obs)
{
  Bdd v0 = mFanins[0]->get_gfunc();
  Bdd v1 = mFanins[1]->get_gfunc();
  Bdd v2 = mFanins[2]->get_gfunc();
  Bdd v3 = mFanins[3]->get_gfunc();
  Bdd l0 = obs;
  Bdd l1 = l0 & v0;
  Bdd l2 = l1 & v1;
  Bdd l3 = l2 & v2;
  Bdd u2 = v3;
  Bdd u1 = u2 & v2;
  Bdd u0 = u1 & v1;
  mFanins[0]->calc_iobsfunc(l0 & u0);
  mFanins[1]->calc_iobsfunc(l1 & u1);
  mFanins[2]->calc_iobsfunc(l2 & u2);
  mFanins[3]->calc_iobsfunc(l3);
}

// @brief 内容をダンプする．
void
SnAnd4::dump(ostream& s) const
{
  s << "AND4(" << mFanins[0]->id();
  s << ", " << mFanins[1]->id();
  s << ", " << mFanins[2]->id();
  s << ", " << mFanins[3]->id();
  s << ")" << endl;
}


//////////////////////////////////////////////////////////////////////
// SnNand
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
SnNand::SnNand(ymuint32 id,
	       const vector<SimNode*>& inputs) :
  SnAnd(id, inputs)
{
}

// @brief デストラクタ
SnNand::~SnNand()
{
}

// @brief 正常値の計算を行う．
tPackedVal
SnNand::_calc_gval()
{
  size_t n = mNfi;
  tPackedVal new_val = mFanins[0]->get_gval();
  for (size_t i = 1; i < n; ++ i) {
    new_val &= mFanins[i]->get_gval();
  }
  return ~new_val;
}

// @brief 故障値の計算を行う．
tPackedVal
SnNand::_calc_fval()
{
  size_t n = mNfi;
  tPackedVal new_val = mFanins[0]->get_fval();
  for (size_t i = 1; i < n; ++ i) {
    new_val &= mFanins[i]->get_fval();
  }
  return ~new_val;
}

// @brief 正常値の計算を行う．
Bdd
SnNand::_calc_gfunc()
{
  size_t n = mNfi;
  Bdd new_val = mFanins[0]->get_gfunc();
  for (size_t i = 1; i < n; ++ i) {
    new_val &= mFanins[i]->get_gfunc();
  }
  return ~new_val;
}

// @brief 故障値の計算を行う．
Bdd
SnNand::_calc_ffunc()
{
  size_t n = mNfi;
  Bdd new_val = mFanins[0]->get_ffunc();
  for (size_t i = 1; i < n; ++ i) {
    new_val &= mFanins[i]->get_ffunc();
  }
  return ~new_val;
}

// @brief 内容をダンプする．
void
SnNand::dump(ostream& s) const
{
  size_t n = mNfi;
  s << "NAND(" << mFanins[0]->id();
  for (size_t i = 1; i < n; ++ i) {
    s << ", " << mFanins[i]->id();
  }
  s << ")" << endl;
}


//////////////////////////////////////////////////////////////////////
// SnNand2
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
SnNand2::SnNand2(ymuint32 id,
		 const vector<SimNode*>& inputs) :
  SnAnd2(id, inputs)
{
}

// @brief デストラクタ
SnNand2::~SnNand2()
{
}

// @brief 正常値の計算を行う．
tPackedVal
SnNand2::_calc_gval()
{
  tPackedVal pat0 = mFanins[0]->get_gval();
  tPackedVal pat1 = mFanins[1]->get_gval();
  return ~(pat0 & pat1);
}

// @brief 故障値の計算を行う．
tPackedVal
SnNand2::_calc_fval()
{
  tPackedVal pat0 = mFanins[0]->get_fval();
  tPackedVal pat1 = mFanins[1]->get_fval();
  return ~(pat0 & pat1);
}

// @brief 正常値の計算を行う．
Bdd
SnNand2::_calc_gfunc()
{
  Bdd pat0 = mFanins[0]->get_gfunc();
  Bdd pat1 = mFanins[1]->get_gfunc();
  return ~(pat0 & pat1);
}

// @brief 故障値の計算を行う．
Bdd
SnNand2::_calc_ffunc()
{
  Bdd pat0 = mFanins[0]->get_ffunc();
  Bdd pat1 = mFanins[1]->get_ffunc();
  return ~(pat0 & pat1);
}

// @brief 内容をダンプする．
void
SnNand2::dump(ostream& s) const
{
  s << "NAND2(" << mFanins[0]->id();
  s << ", " << mFanins[1]->id();
  s << ")" << endl;
}


//////////////////////////////////////////////////////////////////////
// SnNand3
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
SnNand3::SnNand3(ymuint32 id,
		 const vector<SimNode*>& inputs) :
  SnAnd3(id, inputs)
{
}

// @brief デストラクタ
SnNand3::~SnNand3()
{
}

// @brief 正常値の計算を行う．
tPackedVal
SnNand3::_calc_gval()
{
  tPackedVal pat0 = mFanins[0]->get_gval();
  tPackedVal pat1 = mFanins[1]->get_gval();
  tPackedVal pat2 = mFanins[2]->get_gval();
  return ~(pat0 & pat1 & pat2);
}

// @brief 故障値の計算を行う．
tPackedVal
SnNand3::_calc_fval()
{
  tPackedVal pat0 = mFanins[0]->get_fval();
  tPackedVal pat1 = mFanins[1]->get_fval();
  tPackedVal pat2 = mFanins[2]->get_fval();
  return ~(pat0 & pat1 & pat2);
}

// @brief 正常値の計算を行う．
Bdd
SnNand3::_calc_gfunc()
{
  Bdd pat0 = mFanins[0]->get_gfunc();
  Bdd pat1 = mFanins[1]->get_gfunc();
  Bdd pat2 = mFanins[2]->get_gfunc();
  return ~(pat0 & pat1 & pat2);
}

// @brief 故障値の計算を行う．
Bdd
SnNand3::_calc_ffunc()
{
  Bdd pat0 = mFanins[0]->get_ffunc();
  Bdd pat1 = mFanins[1]->get_ffunc();
  Bdd pat2 = mFanins[2]->get_ffunc();
  return ~(pat0 & pat1 & pat2);
}

// @brief 内容をダンプする．
void
SnNand3::dump(ostream& s) const
{
  s << "NAND3(" << mFanins[0]->id();
  s << ", " << mFanins[1]->id();
  s << ", " << mFanins[2]->id();
  s << ")" << endl;
}


//////////////////////////////////////////////////////////////////////
// SnNand4
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
SnNand4::SnNand4(ymuint32 id,
		 const vector<SimNode*>& inputs) :
  SnAnd4(id, inputs)
{
}

// @brief デストラクタ
SnNand4::~SnNand4()
{
}

// @brief 正常値の計算を行う．
tPackedVal
SnNand4::_calc_gval()
{
  tPackedVal pat0 = mFanins[0]->get_gval();
  tPackedVal pat1 = mFanins[1]->get_gval();
  tPackedVal pat2 = mFanins[2]->get_gval();
  tPackedVal pat3 = mFanins[3]->get_gval();
  return ~(pat0 & pat1 & pat2 & pat3);
}

// @brief 故障値の計算を行う．
tPackedVal
SnNand4::_calc_fval()
{
  tPackedVal pat0 = mFanins[0]->get_fval();
  tPackedVal pat1 = mFanins[1]->get_fval();
  tPackedVal pat2 = mFanins[2]->get_fval();
  tPackedVal pat3 = mFanins[3]->get_fval();
  return ~(pat0 & pat1 & pat2 & pat3);
}

// @brief 正常値の計算を行う．
Bdd
SnNand4::_calc_gfunc()
{
  Bdd pat0 = mFanins[0]->get_gfunc();
  Bdd pat1 = mFanins[1]->get_gfunc();
  Bdd pat2 = mFanins[2]->get_gfunc();
  Bdd pat3 = mFanins[3]->get_gfunc();
  return ~(pat0 & pat1 & pat2 & pat3);
}

// @brief 故障値の計算を行う．
Bdd
SnNand4::_calc_ffunc()
{
  Bdd pat0 = mFanins[0]->get_ffunc();
  Bdd pat1 = mFanins[1]->get_ffunc();
  Bdd pat2 = mFanins[2]->get_ffunc();
  Bdd pat3 = mFanins[3]->get_ffunc();
  return ~(pat0 & pat1 & pat2 & pat3);
}

// @brief 内容をダンプする．
void
SnNand4::dump(ostream& s) const
{
  s << "NAND4(" << mFanins[0]->id();
  s << ", " << mFanins[1]->id();
  s << ", " << mFanins[2]->id();
  s << ", " << mFanins[3]->id();
  s << ")" << endl;
}

END_NAMESPACE_YM_SEAL_CVF
