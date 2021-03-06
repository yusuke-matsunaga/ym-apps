﻿
/// @file GbmSolver.cc
/// @brief GbmSolver の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2013 Yusuke Matsunaga
/// All rights reserved.


#include "GbmSolver.h"
#include "RcfNetwork.h"
#include "RcfNode.h"
#include "RcfNodeHandle.h"

#include "GbmSatNaiveBinary.h"
#include "GbmSatNaiveEnum.h"
#include "GbmSatNaiveOneHot.h"
#include "GbmSatCegarBinary.h"
#include "GbmSatCegarEnum.h"
#include "GbmSatCegarOneHot.h"
#include "GbmBddCegarBinary.h"
#include "GbmBddCegarEnum.h"
#include "GbmBddCegarOneHot.h"


BEGIN_NAMESPACE_YM

//////////////////////////////////////////////////////////////////////
// クラス GbmSolver
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
GbmSolver::GbmSolver()
{
  mVerify = false;
  mDebug = false;
}

// @brief デストラクタ
GbmSolver::~GbmSolver()
{
}

// @brief 派生クラスを作るクラスメソッド
// @param[in] method 手法を表す文字列
GbmSolver*
GbmSolver::new_solver(const string& method)
{
  GbmSolver* solver = NULL;
  if ( method == "sat_naive_binary" ) {
    solver = new GbmSatNaiveBinary();
  }
  else if ( method == "sat_naive_enum" ) {
    solver = new GbmSatNaiveEnum();
  }
  else if ( method == "sat_naive_onehot" ) {
    solver = new GbmSatNaiveOneHot();
  }
  else if ( method == "sat_cegar_binary" ) {
    solver = new GbmSatCegarBinary();
  }
  else if ( method == "sat_cegar_enum" ) {
    solver = new GbmSatCegarEnum();
  }
  else if ( method == "sat_cegar_onehot" ) {
    solver = new GbmSatCegarOneHot();
  }
  else if ( method == "bdd_cegar_binary" ) {
    solver = new GbmBddCegarBinary();
  }
  else if ( method == "bdd_cegar_enum" ) {
    solver = new GbmBddCegarEnum();
  }
  else if ( method == "bdd_cegar_onehot" ) {
    solver = new GbmBddCegarOneHot();
  }
  else if ( method == string() ) {
    // default fall-back
    solver = new GbmSatCegarOneHot();
  }

  return solver;
}

// @brief verify フラグを立てる
void
GbmSolver::verify_on()
{
  mVerify = true;
}

// @brief verify フラグを降ろす
void
GbmSolver::verify_off()
{
  mVerify = false;
}

// @brief debug フラグを立てる．
void
GbmSolver::debug_on()
{
  mDebug = true;
}

// @brief debug フラグを降ろす.
void
GbmSolver::debug_off()
{
  mDebug = false;
}

// @brief debug フラグを得る．
bool
GbmSolver::debug() const
{
  return mDebug;
}

// @brief 入力順を考慮したマッチング問題を解く
// @param[in] network RcfNetwork
// @param[in] func マッチング対象の関数
// @param[out] conf_bits configuration ビットの値を収める配列
// @param[out] iorder 入力順序
//             iorder[pos] に network の pos 番めの入力に対応した
//             関数の入力番号が入る．
bool
GbmSolver::solve(const RcfNetwork& network,
		 const TvFunc& func,
		 vector<bool>& conf_bits,
		 vector<ymuint>& iorder,
		 ymuint& loop_count)
{
  ymuint ni = func.input_num();
  vector<ymuint> rep(ni);
  rep[0] = 0;
  for (ymuint i = 1; i < ni; ++ i) {
    rep[i] = i;
    for (ymuint j = i; j > 0; ) {
      -- j;
      if ( func.check_sym(VarId(i), VarId(j)) ) {
	rep[i] = j;
	break;
      }
    }
  }

  bool stat = _solve(network, func, rep, conf_bits, iorder, loop_count);

  if ( mVerify && stat ) {
    // 検証を行う．
    bool vstat = verify(network, func, conf_bits, iorder);
    if ( debug() ) {
      if ( vstat ) {
	cout << "Verify succeeded" << endl;
      }
      else {
	cout << "Verify failed" << endl;
      }
    }
    ASSERT_COND( vstat );
  }

  return stat;
}

// @brief 検証を行う．
// @param[in] network RcfNetwork
// @param[in] func マッチング対象の関数
// @param[in] conf_bits configuration ビットの値を収める配列
// @param[in] iorder 入力順序
//             iorder[pos] に network の pos 番めの入力に対応した
//             関数の入力番号が入る．
bool
GbmSolver::verify(const RcfNetwork& network,
		  const TvFunc& func,
		  const vector<bool>& conf_bits,
		  const vector<ymuint>& iorder)
{
  ymuint nn = network.node_num();
  vector<TvFunc> func_array(nn);

  ymuint npi = func.input_num();
  // これは network.input_num() と等しいはず
  ASSERT_COND( npi == network.input_num() );
  for (ymuint i = 0; i < npi; ++ i) {
    const RcfNode* node = network.input_node(i);
    ymuint id = node->id();
    func_array[id] = TvFunc::posi_literal(npi, VarId(iorder[i]));
  }

  ymuint nf = network.func_node_num();
  for (ymuint i = 0; i < nf; ++ i) {
    const RcfNode* node = network.func_node(i);
    func_array[node->id()] = node->calc_func(func_array, conf_bits);
  }

  RcfNodeHandle output = network.output();
  TvFunc ofunc = func_array[output.id()];
  if ( output.inv() ) {
    ofunc = ~ofunc;
  }

  if ( func != ofunc ) {
    cerr << "verification error" << endl
	 << " original func: " << func << endl
	 << " lut func:      " << ofunc << endl;
    cerr << " iorder: ";
    for (ymuint i = 0; i < npi; ++ i) {
      cerr << " " << iorder[i];
    }
    cerr << endl;
    ymuint nc = conf_bits.size();
    cerr << " conf_bits =    ";
    for (ymuint i = 0; i < nc; ++ i) {
      cerr << conf_bits[i];
    }
    cerr << endl;
    for (ymuint i = 0; i < nf; ++ i) {
      const RcfNode* node = network.func_node(i);
      cerr << "Node#" << node->id() << ": " << func_array[node->id()] << endl;
    }
  }
  return func == ofunc;
}

END_NAMESPACE_YM
