#ifndef GBMINCRONEHOT_H
#define GBMINCRONEHOT_H

/// @file GbmIncrOneHot.h
/// @brief GbmIncrOneHot のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2013 Yusuke Matsunaga
/// All rights reserved.


#include "GbmSolver.h"
#include "GbmLit.h"
#include "RcfNode.h"
#include "ym_logic/SatSolver.h"


BEGIN_NAMESPACE_YM

//////////////////////////////////////////////////////////////////////
/// @class GbmIncr GbmIncr.h "GbmIncr.h"
/// @brief 単純な GbmSolver (one-hot encoding)
//////////////////////////////////////////////////////////////////////
class GbmIncrOneHot :
  public GbmSolver
{
public:

  /// @brief コンストラクタ
  GbmIncrOneHot();

  /// @brief デストラクタ
  virtual
  ~GbmIncrOneHot();


private:
  //////////////////////////////////////////////////////////////////////
  // GbmSolver の仮想関数
  //////////////////////////////////////////////////////////////////////

  /// @brief 入力順を考慮したマッチング問題を解く
  /// @param[in] network RcfNetwork
  /// @param[in] func マッチング対象の関数
  /// @param[out] conf_bits configuration ビットの値を収める配列
  /// @param[out] iorder 入力順序
  /// @note iorder[0] に func の0番めの入力に対応した RcfNetwork の入力番号が入る．
  bool
  _solve(const RcfNetwork& network,
	 const TvFunc& func,
	 vector<bool>& conf_bits,
	 vector<ymuint>& iorder);

};

END_NAMESPACE_YM

#endif // GBMINCRONEHOT_H
