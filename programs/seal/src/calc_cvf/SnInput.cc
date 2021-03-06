﻿
/// @file calc_cvf/SnInput.cc
/// @brief SnInput の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
/// 
/// $Id: SnInput.cc 1978 2009-02-06 12:29:16Z matsunaga $
///
/// Copyright (C) 2005-2008 Yusuke Matsunaga
/// All rights reserved.

#if HAVE_CONFIG_H
#include "seal_config.h"
#endif


#include "SnInput.h"


BEGIN_NAMESPACE_YM_SEAL_CVF

//////////////////////////////////////////////////////////////////////
// SnInput
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
SnInput::SnInput(ymuint32 id) :
  SimNode(id)
{
  set_level(0);
}

// @brief デストラクタ
SnInput::~SnInput()
{
}

// @brief ファンイン数を得る．
size_t
SnInput::nfi() const
{
  return 0;
}

// @brief pos 番めのファンインを得る．
SimNode*
SnInput::fanin(size_t pos) const
{
  ASSERT_NOT_REACHED;
  return NULL;
}

// @brief 正常値の計算を行う．
tPackedVal
SnInput::_calc_gval()
{
  return kPvAll0;
}

// @brief 故障値の計算を行う．
tPackedVal
SnInput::_calc_fval()
{
  return kPvAll0;
}

// @brief 入力の擬似最小 obs を計算する．
void
SnInput::calc_pseudo_min_iobs()
{
}

// @brief 入力の最大 obs を計算する．
void
SnInput::calc_max_iobs(RandGen& randgen)
{
}

// @brief 入力の obs を計算する．
void
SnInput::_calc_iobs(tPackedVal obs)
{
}

// @brief 正常値の計算を行う．
Bdd
SnInput::_calc_gfunc()
{
  ASSERT_NOT_REACHED;
  return Bdd();
}

// @brief 故障値の計算を行う．
Bdd
SnInput::_calc_ffunc()
{
  ASSERT_NOT_REACHED;
  return Bdd();
}

// @brief 入力の擬似最小 obs を計算する．
void
SnInput::calc_pseudo_min_iobsfunc()
{
}
  
// @brief 入力の最大 obs を計算する．
void
SnInput::calc_max_iobsfunc()
{
}
  
// @brief 入力の obs を計算する．
void
SnInput::_calc_iobsfunc(const Bdd& obs)
{
}

// @brief 内容をダンプする．
void
SnInput::dump(ostream& s) const
{
  s << "INPUT" << endl;
}


//////////////////////////////////////////////////////////////////////
// SnBuff
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
SnBuff::SnBuff(ymuint32 id,
	       const vector<SimNode*>& inputs) :
  SnGate1(id, inputs)
{
}

// @brief デストラクタ
SnBuff::~SnBuff()
{
}

// @brief 正常値の計算を行う．
tPackedVal
SnBuff::_calc_gval()
{
  return mFanin->get_gval();
}

// @brief 故障値の計算を行う．
tPackedVal
SnBuff::_calc_fval()
{
  return mFanin->get_fval();
}

// @brief 入力の擬似最小 obs を計算する．
void
SnBuff::calc_pseudo_min_iobs()
{
  mFanin->or_obs(get_obs());
}

// @brief 入力の最大 obs を計算する．
void
SnBuff::calc_max_iobs(RandGen& randgen)
{
  mFanin->or_obs2(get_obs2());
}

// @brief 入力の obs を計算する．
void
SnBuff::_calc_iobs(tPackedVal obs)
{
  mFanin->calc_iobs(obs);
}

// @brief 正常値の計算を行う．
Bdd
SnBuff::_calc_gfunc()
{
  return mFanin->get_gfunc();
}

// @brief 故障値の計算を行う．
Bdd
SnBuff::_calc_ffunc()
{
  return mFanin->get_ffunc();
}

// @brief 入力の擬似最小 obs を計算する．
void
SnBuff::calc_pseudo_min_iobsfunc()
{
  mFanin->or_obsfunc(get_obsfunc());
}
  
// @brief 入力の最大 obs を計算する．
void
SnBuff::calc_max_iobsfunc()
{
  mFanin->or_obsfunc(get_obsfunc());
}
  
// @brief 入力の obs を計算する．
void
SnBuff::_calc_iobsfunc(const Bdd& obs)
{
  mFanin->calc_iobsfunc(obs);
}

// @brief 内容をダンプする．
void
SnBuff::dump(ostream& s) const
{
  s << "BUFF(" << mFanin->id() << ")" << endl;
}


//////////////////////////////////////////////////////////////////////
// SnNot
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
SnNot::SnNot(ymuint32 id,
	     const vector<SimNode*>& inputs) :
  SnBuff(id, inputs)
{
}

// @brief デストラクタ
SnNot::~SnNot()
{
}

// @brief 正常値の計算を行う．
tPackedVal
SnNot::_calc_gval()
{
  return ~mFanin->get_gval();
}

// @brief 故障値の計算を行う．
tPackedVal
SnNot::_calc_fval()
{
  return ~mFanin->get_fval();
}

// @brief 正常値の計算を行う．
Bdd
SnNot::_calc_gfunc()
{
  return ~mFanin->get_gfunc();
}

// @brief 故障値の計算を行う．
Bdd
SnNot::_calc_ffunc()
{
  return ~mFanin->get_ffunc();
}

// @brief 内容をダンプする．
void
SnNot::dump(ostream& s) const
{
  s << "NOT(" << mFanin->id() << ")" << endl;
}

END_NAMESPACE_YM_SEAL_CVF
