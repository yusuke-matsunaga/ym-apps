
/// @file AstLabel.cc
/// @brief AstLabel の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2014 Yusuke Matsunaga
/// All rights reserved.

#include "AstLabel.h"
#include "AstSymbol.h"

#include "YmslCodeList.h"
#include "YmslScope.h"
#include "YmslVM.h"


BEGIN_NAMESPACE_YM_YMSL

//////////////////////////////////////////////////////////////////////
// クラス AstLabel
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] label ラベル
// @param[in] loc ファイル位置
AstLabel::AstLabel(AstSymbol* label,
		   const FileRegion& loc) :
  AstStatement(loc),
  mLabel(label)
{
}

// @brief デストラクタ
AstLabel::~AstLabel()
{
}

// @brief 関数の登録を行う．
// @param[in] parent_scope 親のスコープ
void
AstLabel::phase1(YmslScope* parent_scope)
{
}

// @brief スコープの生成と参照解決を行う．
// @param[in] parent_scope 親のスコープ
void
AstLabel::phase2(YmslScope* parent_scope)
{
}

// @brief 命令コードのサイズを計算する．
ymuint
AstLabel::calc_size()
{
  return 0;
}

// @brief 命令コードを生成する．
// @param[in] driver ドライバ
// @param[in] code_list 命令コードの格納先
// @param[inout] addr 命令コードの現在のアドレス
//
// addr の値は更新される．
void
AstLabel::compile(YmslDriver& driver,
		  YmslCodeList& code_list,
		  Ymsl_INT& addr)
{
}

// @brief 内容を表示する．(デバッグ用)
// @param[in] s 出力ストリーム
// @param[in] indent インデントレベル
void
AstLabel::print(ostream& s,
		ymuint indent) const
{
  print_indent(s, indent);
  s << mLabel->str_val() << ":" << endl;
}

END_NAMESPACE_YM_YMSL