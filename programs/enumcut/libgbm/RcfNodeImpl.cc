﻿
/// @file RcfNodeImpl.cc
/// @brief RcfNodeImpl の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2013 Yusuke Matsunaga
/// All rights reserved.


#include "RcfNodeImpl.h"


BEGIN_NAMESPACE_YM

BEGIN_NONAMESPACE

// @brief RcfNodeHandle を TvFunc に変換する．
// @param[in] h ハンドル
// @param[in] func_array 関数の配列
inline
TvFunc
handle2func(RcfNodeHandle h,
	    const vector<TvFunc>& func_array)
{
  if ( h.inv() ) {
    return ~func_array[h.id()];
  }
  else {
    return func_array[h.id()];
  }
}

// @brief RcfNodeHandle を bool に変換する．
// @param[in] h ハンドル
// @param[in] val_array 値の配列
inline
bool
handle2val(RcfNodeHandle h,
	   const vector<bool>& val_array)
{
  if ( h.inv() ) {
    return !val_array[h.id()];
  }
  else {
    return val_array[h.id()];
  }
}

END_NONAMESPACE


//////////////////////////////////////////////////////////////////////
// クラス RcfNodeImpl
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] id ID番号
RcfNodeImpl::RcfNodeImpl(ymuint id) :
  mId(id)
{
}

// @brief デストラクタ
RcfNodeImpl::~RcfNodeImpl()
{
}

// @brief ID番号を返す．
ymuint
RcfNodeImpl::id() const
{
  return mId;
}

// @brief 外部入力ノードの時 true を返す．
bool
RcfNodeImpl::is_input() const
{
  return false;
}

// @brief ANDノードの時 true を返す．
bool
RcfNodeImpl::is_and() const
{
  return false;
}

// @brief LUTノードの時 true を返す．
bool
RcfNodeImpl::is_lut() const
{
  return false;
}

// @brief MUX ノードの時 true を返す．
bool
RcfNodeImpl::is_mux() const
{
  return false;
}

// @brief 外部入力番号を返す．
// @note is_input() == true の時のみ意味を持つ．
ymuint
RcfNodeImpl::input_id() const
{
  ASSERT_NOT_REACHED;
  return 0;
}

// @brief ファンイン数を返す．
// @note 外部入力ノードの場合は常に0
// @note AND ノードの場合は常に2
ymuint
RcfNodeImpl::fanin_num() const
{
  return 0;
}

// @brief ファンインのハンドルを返す．
// @param[in] pos ファンイン番号 ( 0 <= pos < fanin_num() )
RcfNodeHandle
RcfNodeImpl::fanin(ymuint pos) const
{
  ASSERT_NOT_REACHED;
  return RcfNodeHandle();
}

// @brief LUT/MUX ノードの時の configuration 変数の最初の番号を得る．
ymuint
RcfNodeImpl::conf_base() const
{
  ASSERT_NOT_REACHED;
  return 0;
}

// @brief LUT/MUX ノードの時の configuration 変数の数を得る．
ymuint
RcfNodeImpl::conf_size() const
{
  return 0;
}


//////////////////////////////////////////////////////////////////////
// クラス RcfInputNode
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] id ID番号
// @param[in] input_id 入力番号
RcfInputNode::RcfInputNode(ymuint id,
			   ymuint input_id) :
  RcfNodeImpl(id),
  mInputId(input_id)
{
}

// @brief デストラクタ
RcfInputNode::~RcfInputNode()
{
}

// @brief 外部入力ノードの時 true を返す．
bool
RcfInputNode::is_input() const
{
  return true;
}

// @brief 外部入力番号を返す．
// @note is_input() == true の時のみ意味を持つ．
ymuint
RcfInputNode::input_id() const
{
  return mInputId;
}

// @brief 関数を計算する．
// @param[in] func_array ノード番号をキーにして関数を格納した配列
// @param[in] conf_bits 設定変数番号をキーにして値を格納した配列
TvFunc
RcfInputNode::calc_func(const vector<TvFunc>& func_array,
			const vector<bool>& conf_bits) const
{
  ASSERT_NOT_REACHED;
  return TvFunc::const_zero(0);
}

// @brief 値を計算する．
// @param[in] val_array ノード番号をキーにして値を格納した配列
// @param[in] conf_bits 設定変数番号をキーにして値を格納した配列
bool
RcfInputNode::simulate(const vector<bool>& val_array,
		       const vector<bool>& conf_bits) const
{
  ASSERT_NOT_REACHED;
  return false;
}


//////////////////////////////////////////////////////////////////////
// クラス RcfAndNode
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] id ID番号
// @param[in] fanin0, fanin1 ファンインのハンドル
RcfAndNode::RcfAndNode(ymuint id,
		       RcfNodeHandle fanin0,
		       RcfNodeHandle fanin1) :
  RcfNodeImpl(id)
{
  mFanin[0] = fanin0;
  mFanin[1] = fanin1;
}

// @brief デストラクタ
RcfAndNode::~RcfAndNode()
{
}

// @brief ANDノードの時 true を返す．
bool
RcfAndNode::is_and() const
{
  return true;
}

// @brief ファンイン数を返す．
// @note 外部入力ノードの場合は常に0
// @note AND ノードの場合は常に2
ymuint
RcfAndNode::fanin_num() const
{
  return 2;
}

// @brief ファンインのハンドルを返す．
// @param[in] pos ファンイン番号 ( 0 <= pos < fanin_num() )
RcfNodeHandle
RcfAndNode::fanin(ymuint pos) const
{
  ASSERT_COND( pos < 2 );
  return mFanin[pos];
}

// @brief 関数を計算する．
// @param[in] func_array ノード番号をキーにして関数を格納した配列
// @param[in] conf_bits 設定変数番号をキーにして値を格納した配列
TvFunc
RcfAndNode::calc_func(const vector<TvFunc>& func_array,
		      const vector<bool>& conf_bits) const
{
  TvFunc if0 = handle2func(mFanin[0], func_array);
  TvFunc if1 = handle2func(mFanin[1], func_array);

  return if0 & if1;
}

// @brief 値を計算する．
// @param[in] val_array ノード番号をキーにして値を格納した配列
// @param[in] conf_bits 設定変数番号をキーにして値を格納した配列
bool
RcfAndNode::simulate(const vector<bool>& val_array,
		     const vector<bool>& conf_bits) const
{
  bool val0 = handle2val(mFanin[0], val_array);
  bool val1 = handle2val(mFanin[1], val_array);

  return val0 && val1;
}


//////////////////////////////////////////////////////////////////////
// クラス RcfLutNode
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] id ID番号
// @param[in] conf_base configuration 変数の基底
// @param[in] fanin_list ファンインのリスト
RcfLutNode::RcfLutNode(ymuint id,
		       ymuint conf_base,
		       const vector<RcfNodeHandle>& fanin_list) :
  RcfNodeImpl(id),
  mConfBase(conf_base),
  mFaninNum(fanin_list.size())
{
  for (ymuint i = 0; i < mFaninNum; ++ i) {
    mFanin[i] = fanin_list[i];
  }
}

// @brief デストラクタ
RcfLutNode::~RcfLutNode()
{
}

// @brief LUTノードの時 true を返す．
bool
RcfLutNode::is_lut() const
{
  return true;
}

// @brief ファンイン数を返す．
// @note 外部入力ノードの場合は常に0
// @note AND ノードの場合は常に2
ymuint
RcfLutNode::fanin_num() const
{
  return mFaninNum;
}

// @brief ファンインのハンドルを返す．
// @param[in] pos ファンイン番号 ( 0 <= pos < fanin_num() )
RcfNodeHandle
RcfLutNode::fanin(ymuint pos) const
{
  ASSERT_COND( pos < mFaninNum );
  return mFanin[pos];
}

// @brief LUT/MUX ノードの時の configuration 変数の最初の番号を得る．
ymuint
RcfLutNode::conf_base() const
{
  return mConfBase;
}

// @brief LUT/MUX ノードの時の configuration 変数の数を得る．
ymuint
RcfLutNode::conf_size() const
{
  return (1U << mFaninNum);
}

// @brief 関数を計算する．
// @param[in] func_array ノード番号をキーにして関数を格納した配列
// @param[in] conf_bits 設定変数番号をキーにして値を格納した配列
TvFunc
RcfLutNode::calc_func(const vector<TvFunc>& func_array,
		      const vector<bool>& conf_bits) const
{
  vector<TvFunc> ifuncs(mFaninNum);
  for (ymuint i = 0; i < mFaninNum; ++ i) {
    ifuncs[i] = handle2func(mFanin[i], func_array);
  }
  ymuint ni = ifuncs[0].input_num();
  TvFunc func = TvFunc::const_zero(ni);
  ymuint nexp = 1U << mFaninNum;
  for (ymuint b = 0; b < nexp; ++ b) {
    if ( conf_bits[mConfBase + b] ) {
      TvFunc prod = TvFunc::const_one(ni);
      for (ymuint i = 0; i < mFaninNum; ++ i) {
	if ( b & (1U << i) ) {
	  prod &= ifuncs[i];
	}
	else {
	  prod &= ~ifuncs[i];
	}
      }
      func |= prod;
    }
  }
  return func;
}

// @brief 値を計算する．
// @param[in] val_array ノード番号をキーにして値を格納した配列
// @param[in] conf_bits 設定変数番号をキーにして値を格納した配列
bool
RcfLutNode::simulate(const vector<bool>& val_array,
		     const vector<bool>& conf_bits) const
{
  ymuint pos = 0U;
  for (ymuint i = 0; i < mFaninNum; ++ i) {
    if ( handle2val(mFanin[i], val_array) ) {
      pos |= (1U << i);
    }
  }
  return conf_bits[mConfBase + pos];
}


//////////////////////////////////////////////////////////////////////
// クラス RcfMuxNode
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] id ID番号
// @param[in] conf_base configuration 変数の基底
// @param[in] fanin_list ファンインのリスト
RcfMuxNode::RcfMuxNode(ymuint id,
		       ymuint conf_base,
		       const vector<RcfNodeHandle>& fanin_list) :
  RcfNodeImpl(id),
  mConfBase(conf_base),
  mFaninNum(fanin_list.size())
{
  for (ymuint i = 0; i < mFaninNum; ++ i) {
    mFanin[i] = fanin_list[i];
  }

  // ファンインを区別するのに必要なビット数 ( = mConfSize ) を計算する．
  for (ymuint tmp = 1U, mConfSize = 0; tmp < mFaninNum; ++ mConfSize, tmp <<= 1) ;
}

// @brief デストラクタ
RcfMuxNode::~RcfMuxNode()
{
}

// @brief MUX ノードの時 true を返す．
bool
RcfMuxNode::is_mux() const
{
  return true;
}

// @brief ファンイン数を返す．
// @note 外部入力ノードの場合は常に0
// @note AND ノードの場合は常に2
ymuint
RcfMuxNode::fanin_num() const
{
  return mFaninNum;
}

// @brief ファンインのハンドルを返す．
// @param[in] pos ファンイン番号 ( 0 <= pos < fanin_num() )
RcfNodeHandle
RcfMuxNode::fanin(ymuint pos) const
{
  ASSERT_COND( pos < mFaninNum );
  return mFanin[pos];
}

// @brief LUT/MUX ノードの時の configuration 変数の最初の番号を得る．
ymuint
RcfMuxNode::conf_base() const
{
  return mConfBase;
}

// @brief LUT/MUX ノードの時の configuration 変数の数を得る．
ymuint
RcfMuxNode::conf_size() const
{
  return mConfSize;
}

// @brief 関数を計算する．
// @param[in] func_array ノード番号をキーにして関数を格納した配列
// @param[in] conf_bits 設定変数番号をキーにして値を格納した配列
TvFunc
RcfMuxNode::calc_func(const vector<TvFunc>& func_array,
		      const vector<bool>& conf_bits) const
{
  ymuint pos = 0U;
  for (ymuint i = 0; i < mConfSize; ++ i) {
    if ( conf_bits[mConfBase + i] ) {
      pos += (1U << i);
    }
  }
  ASSERT_COND( pos < mFaninNum );
  return handle2func(mFanin[pos], func_array);
}

// @brief 値を計算する．
// @param[in] val_array ノード番号をキーにして値を格納した配列
// @param[in] conf_bits 設定変数番号をキーにして値を格納した配列
bool
RcfMuxNode::simulate(const vector<bool>& val_array,
		     const vector<bool>& conf_bits) const
{
  ymuint pos = 0U;
  for (ymuint i = 0; i < mConfSize; ++ i) {
    if ( conf_bits[mConfBase + i] ) {
      pos += (1U << i);
    }
  }
  ASSERT_COND( pos < mFaninNum );
  return handle2val(mFanin[pos], val_array);
}

END_NAMESPACE_YM
