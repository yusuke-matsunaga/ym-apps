﻿#ifndef RCFNODE_H
#define RCFNODE_H

/// @file RcfNode.h
/// @brief RcfNode のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2013 Yusuke Matsunaga
/// All rights reserved.


#include "RcfNodeHandle.h"
#include "YmLogic/TvFunc.h"


BEGIN_NAMESPACE_YM

//////////////////////////////////////////////////////////////////////
/// @class RcfNode RcfNode.h "RcfNode.h"
/// @brief Rcf で用いるノードを表すクラス
//////////////////////////////////////////////////////////////////////
class RcfNode
{
public:

  /// @brief デストラクタ
  virtual
  ~RcfNode() { }


public:
  //////////////////////////////////////////////////////////////////////
  // 外部インターフェイス
  //////////////////////////////////////////////////////////////////////

  /// @brief ID番号を返す．
  virtual
  ymuint
  id() const = 0;

  /// @brief 外部入力ノードの時 true を返す．
  virtual
  bool
  is_input() const = 0;

  /// @brief ANDノードの時 true を返す．
  virtual
  bool
  is_and() const = 0;

  /// @brief LUTノードの時 true を返す．
  virtual
  bool
  is_lut() const = 0;

  /// @brief MUX ノードの時 true を返す．
  virtual
  bool
  is_mux() const = 0;

  /// @brief 外部入力番号を返す．
  /// @note is_input() == true の時のみ意味を持つ．
  virtual
  ymuint
  input_id() const = 0;

  /// @brief ファンイン数を返す．
  /// @note 外部入力ノードの場合は常に0
  /// @note AND ノードの場合は常に2
  virtual
  ymuint
  fanin_num() const = 0;

  /// @brief ファンインのハンドルを返す．
  /// @param[in] pos ファンイン番号 ( 0 <= pos < fanin_num() )
  virtual
  RcfNodeHandle
  fanin(ymuint pos) const = 0;

  /// @brief LUT/MUX ノードの時の configuration 変数の最初の番号を得る．
  virtual
  ymuint
  conf_base() const = 0;

  /// @brief LUT/MUX ノードの時の configuration 変数の数を得る．
  virtual
  ymuint
  conf_size() const = 0;

  /// @brief 関数を計算する．
  /// @param[in] func_array ノード番号をキーにして関数を格納した配列
  /// @param[in] conf_bits 設定変数番号をキーにして値を格納した配列
  virtual
  TvFunc
  calc_func(const vector<TvFunc>& func_array,
	    const vector<bool>& conf_bits) const = 0;

  /// @brief 値を計算する．
  /// @param[in] val_array ノード番号をキーにして値を格納した配列
  /// @param[in] conf_bits 設定変数番号をキーにして値を格納した配列
  virtual
  bool
  simulate(const vector<bool>& val_array,
	   const vector<bool>& conf_bits) const = 0;


};

END_NAMESPACE_YM

#endif // RCFNODE_H
