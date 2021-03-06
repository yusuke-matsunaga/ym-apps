﻿#ifndef NPNXFORM_H
#define NPNXFORM_H

/// @file NpnXform.h
/// @brief NpnXform のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2012 Yusuke Matsunaga
/// All rights reserved.


#include "YmTools.h"


BEGIN_NAMESPACE_YM

//////////////////////////////////////////////////////////////////////
/// @class NpnXform NpnXform.h "NpnXform.h"
/// @brief NPN 変換を表すクラス(4入力限定)
///
/// 順列は全部で (4 x 3 x 2 x 1) = 24通りしかないので
/// 0 - 23 までのIDに対応付ける．
/// そのため順列同士の演算はすべて表引きで行なう．
/// 符号化は以下の通り
/// - 0      bit目: 出力の反転
/// - 1 -  4 bit目: 入力の反転
/// - 5 - 10 bit目: 順列
//////////////////////////////////////////////////////////////////////
class NpnXform
{
public:

  /// @brief 空のコンストラクタ
  /// @note 恒等変換になる．
  NpnXform();

  /// @brief 順列番号と極性ビットベクタを指定したコンストラクタ
  /// @param[in] pid 順列番号 ( 0 <= pid < 24 )
  /// @param[in] pols 極性ビットベクタ ( 0 <= pols < 32 )
  NpnXform(ymuint pid,
	   ymuint pols);

  /// @brief 生のデータを引数にしたコンストラクタ
  /// @note 危険
  explicit
  NpnXform(ymuint data);


public:
  //////////////////////////////////////////////////////////////////////
  // 情報を取り出す関数
  //////////////////////////////////////////////////////////////////////

  /// @brief 入力の置換を得る．
  /// @param[in] pos 元の入力位置 ( 0 <= pos < 4 )
  /// @return 置換先の位置
  ymuint
  input_perm(ymuint pos) const;

  /// @brief 入力の極性を得る．
  /// @param[in] pos 元の入力位置 ( 0 <= pos < 4 )
  /// @retval true 反転あり
  /// @retval false 反転なし
  bool
  input_inv(ymuint pos) const;

  /// @brief 出力の極性を得る．
  bool
  output_inv() const;

  /// @brief 与えられた関数のサポートに関する同値類の代表変換を求める．
  /// @param[in] sup_vec サポートベクタ
  /// @return 正規化後の自分自身を返す．
  /// @note サポートに含まれていない変数の変換を消去する．
  NpnXform
  rep(ymuint8 sup_vec) const;

  /// @brief 生のデータを取り出す．
  /// @note 値域は 0 - 1023
  ymuint
  data() const;


public:
  //////////////////////////////////////////////////////////////////////
  // 内容を設定する関数
  //////////////////////////////////////////////////////////////////////

  /// @brief 入力の反転属性を反転させる．
  void
  flip_iinv(ymuint pos);

  /// @brief 出力の反転属性を反転させる．
  void
  flip_oinv();


public:
  //////////////////////////////////////////////////////////////////////
  // クラスメソッド
  //////////////////////////////////////////////////////////////////////

  /// @brief 入力の順列から順列番号を得る．
  static
  ymuint
  perm_id(ymuint perm[]);


public:
  //////////////////////////////////////////////////////////////////////
  // 演算
  //////////////////////////////////////////////////////////////////////

  /// @brief 合成する．
  friend
  NpnXform
  operator*(NpnXform left,
	    NpnXform right);

  /// @brief 合成する．
  const NpnXform&
  operator*=(NpnXform right);

  /// @brief サポートベクタに対する変換
  /// @note といっても ymuint なのでちょっと危険
  friend
  ymuint
  operator*(ymuint sup_vec,
	    NpnXform xform);

  /// @brief 正規化する．
  /// @param[in] sup サポート数
  /// @return 正規化後の自分自身を返す．
  /// @note サポートに含まれていない変数の変換を消去する．
  const NpnXform&
  normalize(ymuint sup);

  /// @brief 逆変換を求める．
  friend
  NpnXform
  inverse(NpnXform left);

  /// @brief マージする．
  friend
  bool
  merge(NpnXform left,
	ymuint left_sup,
	NpnXform right,
	ymuint right_sup,
	NpnXform& result);

  /// @brief 等価比較
  friend
  bool
  operator==(NpnXform left,
	     NpnXform right);

  /// @brief 大小比較
  friend
  bool
  operator<(NpnXform left,
	    NpnXform right);


private:
  //////////////////////////////////////////////////////////////////////
  // 下請け関数
  //////////////////////////////////////////////////////////////////////

  /// @brief 順列番号を取り出す．
  ymuint
  get_perm() const;

  /// @brief 極性ベクタを取り出す．
  ymuint
  get_pols() const;


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // 実体
  ymuint16 mData;

};

/// @brief 等価比較
/// @param[in] left, right オペランド
bool
operator==(NpnXform left,
	   NpnXform right);

/// @brief 非等価比較
/// @param[in] left, right オペランド
bool
operator!=(NpnXform left,
	   NpnXform right);

/// @brief 大小比較 ( < )
/// @param[in] left, right オペランド
bool
operator<(NpnXform left,
	  NpnXform right);

/// @brief 大小比較 ( > )
/// @param[in] left, right オペランド
bool
operator>(NpnXform left,
	  NpnXform right);

/// @brief 大小比較 ( <= )
/// @param[in] left, right オペランド
bool
operator<=(NpnXform left,
	   NpnXform right);

/// @brief 大小比較 ( >= )
/// @param[in] left, right オペランド
bool
operator>=(NpnXform left,
	   NpnXform right);

/// @brief 関数ベクタを変換する．
/// @param[in] left, right オペランド
ymuint16
xform_func(ymuint16 func,
	   NpnXform xf);

/// @brief 内容を表示する．
/// @param[in] left, right オペランド
ostream&
operator<<(ostream& s,
	   NpnXform xf);


//////////////////////////////////////////////////////////////////////
// インライン関数の定義
//////////////////////////////////////////////////////////////////////

// @brief 空のコンストラクタ
// @note 恒等変換になる．
inline
NpnXform::NpnXform() :
  mData(0)
{
}

// @brief 順列番号と極性ビットベクタを指定したコンストラクタ
// @param[in] pid 順列番号 ( 0 <= pid < 24 )
// @param[in] pols 極性ビットベクタ ( 0 <= pols < 32 )
inline
NpnXform::NpnXform(ymuint pid,
		   ymuint pols) :
  mData((pid << 5) | pols)
{
}

// @brief 生のデータを引数にしたコンストラクタ
// @note 危険
inline
NpnXform::NpnXform(ymuint data) :
  mData(data)
{
}

// @brief 入力の極性を得る．
// @param[in] pos 元の入力位置 ( 0 <= pos < 4 )
// @retval true 反転あり
// @retval false 反転なし
inline
bool
NpnXform::input_inv(ymuint pos) const
{
  return static_cast<bool>((mData >> (pos + 1)) & 1U);
}

// @brief 出力の極性を得る．
inline
bool
NpnXform::output_inv() const
{
  return static_cast<bool>(mData & 1U);
}

// @brief 生のデータを取り出す．
inline
ymuint
NpnXform::data() const
{
  return mData;
}

// @brief 入力の反転属性を反転させる．
inline
void
NpnXform::flip_iinv(ymuint pos)
{
  mData ^= (1U << (pos + 1));
}

// @brief 出力の反転属性を反転させる．
inline
void
NpnXform::flip_oinv()
{
  mData ^= 1U;
}

// @brief 合成する．
inline
NpnXform
operator*(NpnXform left,
	  NpnXform right)
{
  return NpnXform(left).operator*=(right);
}

// @brief 等価比較
// @param[in] left, right オペランド
inline
bool
operator==(NpnXform left,
	   NpnXform right)
{
  return left.mData == right.mData;
}

// @brief 非等価比較
// @param[in] left, right オペランド
inline
bool
operator!=(NpnXform left,
	   NpnXform right)
{
  return !operator==(left, right);
}

// @brief 大小比較 ( < )
// @param[in] left, right オペランド
inline
bool
operator<(NpnXform left,
	  NpnXform right)
{
  return left.mData < right.mData;
}

// @brief 大小比較 ( > )
// @param[in] left, right オペランド
inline
bool
operator>(NpnXform left,
	  NpnXform right)
{
  return operator<(right, left);
}

// @brief 大小比較 ( <= )
// @param[in] left, right オペランド
inline
bool
operator<=(NpnXform left,
	   NpnXform right)
{
  return !operator<(right, left);
}

// @brief 大小比較 ( >= )
// @param[in] left, right オペランド
inline
bool
operator>=(NpnXform left,
	   NpnXform right)
{
  return !operator<(left, right);
}

// @brief 順列番号を取り出す．
inline
ymuint
NpnXform::get_perm() const
{
  return (mData >> 5) & 31U;
}

// @brief 極性ベクタを取り出す．
inline
ymuint
NpnXform::get_pols() const
{
  return mData & 31U;
}

// @brief 与えられた関数のサポートに関する同値類の代表変換を求める．
// @param[in] sup_vec サポートベクタ
// @return 正規化後の自分自身を返す．
// @note サポートに含まれていない変数の変換を消去する．
inline
NpnXform
NpnXform::rep(ymuint8 sup_vec) const
{
  return NpnXform(*this).normalize(sup_vec);
}

END_NAMESPACE_YM

#endif // NPNXFORM_H
