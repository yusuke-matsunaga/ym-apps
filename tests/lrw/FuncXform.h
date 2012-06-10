#ifndef FUNCXORM_H
#define FUNCXORM_h

/// @file FuncXform.h
/// @brief FuncXform のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2012 Yusuke Matsunaga
/// All rights reserved.


#include "ymtools.h"
#include "NpnXform.h"


BEGIN_NAMESPACE_YM

//////////////////////////////////////////////////////////////////////
/// @class FuncXform FuncXform.h "FuncXform.h"
/// @brief 関数変換を表すクラス
//////////////////////////////////////////////////////////////////////
struct FuncXform
{

  /// @brief コンストラクタ
  FuncXform(ymuint16 fv,
	    ymuint16 perm) :
    mVector(fv),
    mXf(perm)
  {
  }

  // 変換先の関数ベクタ
  ymuint16 mVector;

  // 変換方法
  NpnXform mXf;

};

END_NAMESPACE_YM

#endif // FUNCXORM_H
