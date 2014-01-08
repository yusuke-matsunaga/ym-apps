#ifndef GBMENGINE_H
#define GBMENGINE_H

/// @file GbmEngine.h
/// @brief GbmEngine のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2013, 2014 Yusuke Matsunaga
/// All rights reserved.


#include "ymtools.h"
#include "GbmLit.h"
#include "RcfNode.h"
#include "ym_logic/SatSolver.h"


BEGIN_NAMESPACE_YM

//////////////////////////////////////////////////////////////////////
/// @class GbmEngine GbmEngine.h "GbmEngine.h"
/// @brief GBM の基本処理を行なうクラス
//////////////////////////////////////////////////////////////////////
class GbmEngine
{
public:

  /// @brief コンストラクタ
  /// @param[in] solver SATソルバ
  /// @param[in] node_num ノード数
  /// @param[in] conf_num 設定変数の数
  GbmEngine(SatSolver& solver,
	    ymuint node_num,
	    ymuint conf_num);

  /// @brief デストラクタ
  ~GbmEngine();


public:
  //////////////////////////////////////////////////////////////////////
  // 外部インターフェイス
  //////////////////////////////////////////////////////////////////////

  /// @brief ノードに対応するリテラルを登録する．
  /// @param[in] id ノード番号
  /// @param[in] lit リテラル
  void
  set_node_var(ymuint id,
	       GbmLit lit);

  /// @brief 設定変数に対応するリテラルを登録する．
  /// @param[in] id 変数番号
  /// @param[in] lit リテラル
  void
  set_conf_var(ymuint id,
	       GbmLit lit);

  /// @brief ノードの入出力の関係を表す CNF 式を作る．
  /// @param[in] node 対象のノード
  /// @return 割り当てが矛盾を起こしたら false を返す．
  bool
  make_node_cnf(const RcfNode* node);


private:
  //////////////////////////////////////////////////////////////////////
  // 内部で用いられる関数
  //////////////////////////////////////////////////////////////////////

  /// @brief AND ゲートを表す節を追加する．
  /// @param[in] input_vars 入力変数のリスト
  /// @param[in] output_var 出力変数
  /// @return 割り当てが矛盾を起こしたら false を返す．
  bool
  make_AND(const vector<GbmLit>& input_vars,
	   GbmLit output_var);

  /// @brief LUT を表す節を追加する．
  /// @param[in] input_vars 入力変数のリスト
  /// @param[in] lut_vars LUT変数のリスト
  /// @param[in] output_var 出力変数
  /// @note lut_vars のサイズは input_vars のサイズの指数乗
  /// @return 割り当てが矛盾を起こしたら false を返す．
  bool
  make_LUT(const vector<GbmLit>& input_vars,
	   const vector<GbmLit>& lut_vars,
	   GbmLit output_var);

  /// @brief MUX を表す節を追加する．
  /// @param[in] d_vars データ入力変数のリスト
  /// @param[in] s_vars 選択信号変数のリスト
  /// @param[in] output_var 出力変数
  /// @note d_vars のサイズは s_vars のサイズの指数乗
  /// @return 割り当てが矛盾を起こしたら false を返す．
  bool
  make_MUX(const vector<GbmLit>& d_vars,
	   const vector<GbmLit>& s_vars,
	   GbmLit output_var);

  /// @brief RcfNodeHandle から GbmLit を作る．
  /// @param[in] handle ハンドル
  GbmLit
  handle_to_lit(RcfNodeHandle handle);


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // SATソルバ
  SatSolver& mSolver;

  // ノードIDをキーにしてリテラルを格納する配列
  vector<GbmLit> mNodeVarArray;

  // 設定変数番号をキーにしてリテラルを格納する配列
  vector<GbmLit> mConfVarArray;

};

END_NAMESPACE_YM

#endif // GBMENGINE_H
