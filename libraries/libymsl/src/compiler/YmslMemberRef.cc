
/// @file YmslMemberRef.cc
/// @brief YmslMemberRef の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2015 Yusuke Matsunaga
/// All rights reserved.


#include "YmslMemberRef.h"


BEGIN_NAMESPACE_YM_YMSL

//////////////////////////////////////////////////////////////////////
// クラス YmslMemberRef
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] body 本体
// @param[in] offset メンバのオフセット
YmslMemberRef::YmslMemberRef(YmslLeaf* body,
			     ymuint offset) :
  mBody(body),
  mOffset(offset)
{
}

// @brief デストラクタ
YmslMemberRef::~YmslMemberRef()
{
}

// @brief 型を返す．
LeafType
YmslMemberRef::leaf_type() const
{
  return kMemberRef;
}

// @brief 本体を返す．
//
// kMemberRef, kArrayRef, kFuncCall のみ有効
YmslLeaf*
YmslMemberRef::body() const
{
  return mBody;
}

// @brief メンバのオフセットを得る．
//
// kMemberRef のみ有効
ymuint
YmslMemberRef::member_offset() const
{
  return mOffset;
}

END_NAMESPACE_YM_YMSL
