#ifndef LITMAP_H
#define LITMAP_H

/// @file LitMap.h
/// @brief LitMap のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2010, 2012-2014 Yusuke Matsunaga
/// All rights reserved.


#include "satpg_nsdef.h"
#include "logic/Literal.h"


BEGIN_NAMESPACE_YM_SATPG

//////////////////////////////////////////////////////////////////////
/// @class LitMap LitMap.h "LitMap.h"
/// @brief make_gate_cnf で用いられるファンクター
//////////////////////////////////////////////////////////////////////
struct LitMap
{

  /// @brief 出力のリテラルを返す．
  virtual
  Literal
  output() const = 0;

  /// @brief 入力のリテラルを返す．
  virtual
  Literal
  input(ymuint pos) const = 0;

  /// @brief 入力数を返す．
  virtual
  ymuint
  input_size() const = 0;

};


//////////////////////////////////////////////////////////////////////
/// @class GvarLitMap LitMap.h "LitMap.h"
/// @brief TpgNode->gvar() を用いた LitMap
//////////////////////////////////////////////////////////////////////
class GvarLitMap :
  public LitMap
{
public:

  /// @brief コンストラクタ
  GvarLitMap(const TpgNode* node);

  /// @brief 出力のリテラルを返す．
  virtual
  Literal
  output() const;

  /// @brief 入力のリテラルを返す．
  virtual
  Literal
  input(ymuint pos) const;

  /// @brief 入力数を返す．
  virtual
  ymuint
  input_size() const;


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // ノード
  const TpgNode* mNode;

};


//////////////////////////////////////////////////////////////////////
/// @class FvarLitMap LitMap.h "LitMap.h"
/// @brief TpgNode->fvar() を用いた LitMap
//////////////////////////////////////////////////////////////////////
class FvarLitMap :
  public LitMap
{
public:

  /// @brief コンストラクタ
  FvarLitMap(const TpgNode* node);

  /// @brief 出力のリテラルを返す．
  virtual
  Literal
  output() const;

  /// @brief 入力のリテラルを返す．
  virtual
  Literal
  input(ymuint pos) const;

  /// @brief 入力数を返す．
  virtual
  ymuint
  input_size() const;


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // ノード
  const TpgNode* mNode;

};


//////////////////////////////////////////////////////////////////////
/// @class Fvar2LitMap LitMap.h "LitMap.h"
/// @brief 出力の変数を指定した FvarLitMap
//////////////////////////////////////////////////////////////////////
class Fvar2LitMap :
  public FvarLitMap
{
public:

  /// @brief コンストラクタ
  Fvar2LitMap(const TpgNode* node,
	      VarId ovar);

  /// @brief 出力のリテラルを返す．
  virtual
  Literal
  output() const;


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // 出力の変数
  VarId mOvar;

};

END_NAMESPACE_YM_SATPG

#endif // LITMAP_H