﻿#ifndef CALC_SVF_SNXOR_H
#define CALC_SVF_SNXOR_H

/// @file calc_svf/SnXor.h
/// @brief SnXor のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
/// 
/// $Id: SnXor.h 1920 2008-12-20 15:52:42Z matsunaga $
///
/// Copyright (C) 2005-2008 Yusuke Matsunaga
/// All rights reserved.

#include "SnGate.h"


BEGIN_NAMESPACE_YM_SEAL_SVF

//////////////////////////////////////////////////////////////////////
/// @class SnXor SimNode.h
/// @brief XORノード
//////////////////////////////////////////////////////////////////////
class SnXor :
  public SnGate
{
public:

  /// @brief コンストラクタ
  SnXor(ymuint32 id,
	const vector<SimNode*>& inputs);

  /// @brief デストラクタ
  virtual
  ~SnXor();


public:

  /// @brief 正常値の計算を行う．
  virtual
  tPackedVal
  _calc_gval();

  /// @brief 故障値の計算を行う．
  virtual
  tPackedVal
  _calc_fval();

  /// @brief 入力の擬似最小 obs を計算する．
  virtual
  void
  calc_pseudo_min_iobs();
  
  /// @brief 入力の最大 obs を計算する．
  virtual
  void
  calc_max_iobs();
  
  /// @brief 入力の obs を計算する．
  virtual
  void
  _calc_iobs(tPackedVal obs);

  /// @brief 内容をダンプする．
  virtual
  void
  dump(ostream& s) const;

};


//////////////////////////////////////////////////////////////////////
/// @class SnNor2 SimNode.h
/// @brief 2入力XORノード
//////////////////////////////////////////////////////////////////////
class SnXor2 :
  public SnGate2
{
public:

  /// @brief コンストラクタ
  SnXor2(ymuint32 id,
	 const vector<SimNode*>& inputs);

  /// @brief デストラクタ
  virtual
  ~SnXor2();


public:

  /// @brief 正常値の計算を行う．
  virtual
  tPackedVal
  _calc_gval();

  /// @brief 故障値の計算を行う．
  virtual
  tPackedVal
  _calc_fval();

  /// @brief 入力の擬似最小 obs を計算する．
  virtual
  void
  calc_pseudo_min_iobs();
  
  /// @brief 入力の最大 obs を計算する．
  virtual
  void
  calc_max_iobs();
  
  /// @brief 入力の obs を計算する．
  virtual
  void
  _calc_iobs(tPackedVal obs);

  /// @brief 内容をダンプする．
  virtual
  void
  dump(ostream& s) const;

};


//////////////////////////////////////////////////////////////////////
/// @class SnXnor SimNode.h
/// @brief XNORノード
//////////////////////////////////////////////////////////////////////
class SnXnor :
  public SnXor
{
public:

  /// @brief コンストラクタ
  SnXnor(ymuint32 id,
	 const vector<SimNode*>& inputs);

  /// @brief デストラクタ
  virtual
  ~SnXnor();


public:

  /// @brief 正常値の計算を行う．
  virtual
  tPackedVal
  _calc_gval();

  /// @brief 故障値の計算を行う．
  virtual
  tPackedVal
  _calc_fval();

  /// @brief 内容をダンプする．
  virtual
  void
  dump(ostream& s) const;

};


//////////////////////////////////////////////////////////////////////
/// @class SnNor2 SimNode.h
/// @brief 2入力XNORノード
//////////////////////////////////////////////////////////////////////
class SnXnor2 :
  public SnXor2
{
public:

  /// @brief コンストラクタ
  SnXnor2(ymuint32 id,
	  const vector<SimNode*>& inputs);

  /// @brief デストラクタ
  virtual
  ~SnXnor2();


public:

  /// @brief 正常値の計算を行う．
  virtual
  tPackedVal
  _calc_gval();

  /// @brief 故障値の計算を行う．
  virtual
  tPackedVal
  _calc_fval();

  /// @brief 内容をダンプする．
  virtual
  void
  dump(ostream& s) const;

};

END_NAMESPACE_YM_SEAL_SVF

#endif // CALC_SVF_SNXOR_H
