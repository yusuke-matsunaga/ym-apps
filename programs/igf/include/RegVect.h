﻿#ifndef REGVECT_H
#define REGVECT_H

/// @file RegVect.h
/// @brief RegVect のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2013 Yusuke Matsunaga
/// All rights reserved.


#include "igf_nsdef.h"


BEGIN_NAMESPACE_YM_IGF

//////////////////////////////////////////////////////////////////////
/// @class RegVect RegVect.h "RegVect.h"
/// @brief 登録ベクタを表すクラス
///
/// 実際に確保されるサイズは sizeof(RegVect) と異なるので RvMgr 以外が
/// このクラスのインスタンスを生成することを禁止している．
//////////////////////////////////////////////////////////////////////
class RegVect
{
  friend class RvMgr;
private:

  /// @brief コンストラクタ
  /// @param[in] size ベクタのサイズ
  /// @param[in] index インデックス
  RegVect(ymuint size,
	  ymuint index);

  /// @brief デストラクタ
  ~RegVect();


public:
  //////////////////////////////////////////////////////////////////////
  // 外部インターフェイス
  //////////////////////////////////////////////////////////////////////

  /// @brief ベクタの長さを返す．
  ymuint
  size() const;

  /// @brief インデックスを返す．
  ymuint
  index() const;

  /// @brief 値を返す．
  /// @param[in] pos 位置 ( 0 <= pos < size() )
  /// @note 返される値は 0 か 1 のみ
  ymuint
  val(ymuint pos) const;

  /// @brief ハッシュ値を返す．
  ymuint
  hash() const;

  /// @brief 内容を出力する．
  /// @param[in] s 出力先のストリーム
  void
  dump(ostream& s) const;


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // サイズ
  ymuint mSize;

  // インデックス
  ymuint mIndex;

  // ベクタの本体
  // 実際には必要なサイズが確保される．
  ymuint64 mBody[1];

};


//////////////////////////////////////////////////////////////////////
// インライン関数の定義
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] size ベクタのサイズ
// @param[in] index インデックス
inline
RegVect::RegVect(ymuint size,
		 ymuint index) :
  mSize(size),
  mIndex(index)
{
}

// @brief デストラクタ
inline
RegVect::~RegVect()
{
}

// @brief ベクタの長さを返す．
inline
ymuint
RegVect::size() const
{
  return mSize;
}

// @brief インデックスを返す．
inline
ymuint
RegVect::index() const
{
  return mIndex;
}

// @brief 値を返す．
// @param[in] pos 位置 ( 0 <= pos < size() )
// @note 返される値は 0 か 1 のみ
inline
ymuint
RegVect::val(ymuint pos) const
{
  ASSERT_COND( pos < size() );
  ymuint nblk = pos / 64;
  ymuint shft = pos % 64;
  return (mBody[nblk] >> shft) & 1U;
}

END_NAMESPACE_YM_IGF

#endif // REGVECT_H
