
/// @file AstBlockStmt.cc
/// @brief AstBlockStmt の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2014 Yusuke Matsunaga
/// All rights reserved.

#include "AstBlockStmt.h"
#include "AstList.h"

#include "YmslCodeList.h"
#include "YmslScope.h"
#include "YmslVM.h"


BEGIN_NAMESPACE_YM_YMSL

//////////////////////////////////////////////////////////////////////
// クラス AstBlockStmt
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] stmt_list 本体の文
// @param[in] loc ファイル位置
AstBlockStmt::AstBlockStmt(AstStmtList* stmt_list,
			   const FileRegion& loc) :
  AstStatement(loc),
  mStmtList(stmt_list->size()),
  mScope(NULL)
{
  ymuint pos = 0;
  for (AstStmtList::Iterator p = stmt_list->begin();
       !p.is_end(); p.next()) {
    mStmtList[pos] = *p;
    ++ pos;
  }
}

// @brief デストラクタ
AstBlockStmt::~AstBlockStmt()
{
}

// @brief 関数の登録を行う．
// @param[in] parent_scope 親のスコープ
void
AstBlockStmt::phase1(YmslScope* parent_scope)
{
}

// @brief スコープの生成と参照解決を行う．
// @param[in] parent_scope 親のスコープ
void
AstBlockStmt::phase2(YmslScope* parent_scope)
{
  mScope = new YmslScope(parent_scope);
  ymuint n = mStmtList.size();
  for (ymuint i = 0; i < n; ++ i) {
    AstStatement* stmt = mStmtList[i];
    stmt->phase2(mScope);
  }
}

// @brief 命令コードのサイズを計算する．
ymuint
AstBlockStmt::calc_size()
{
  ymuint size = 0;
  for (ymuint pos = 0; pos < mStmtList.size(); ++ pos) {
    size += mStmtList[pos]->calc_size();
  }
  return size;
}

// @brief 命令コードを生成する．
// @param[in] driver ドライバ
// @param[in] code_list 命令コードの格納先
// @param[inout] addr 命令コードの現在のアドレス
//
// addr の値は更新される．
void
AstBlockStmt::compile(YmslDriver& driver,
		      YmslCodeList& code_list,
		      Ymsl_INT& addr)
{
  for (ymuint pos = 0; pos < mStmtList.size(); ++ pos) {
    mStmtList[pos]->compile(driver, code_list, addr);
  }
}

// @brief 内容を表示する．(デバッグ用)
// @param[in] s 出力ストリーム
// @param[in] indent インデントレベル
void
AstBlockStmt::print(ostream& s,
		    ymuint indent) const
{
  ymuint n = mStmtList.size();
  for (ymuint i = 0; i < n; ++ i) {
    AstStatement* stmt = mStmtList[i];
    stmt->print(s, indent);
  }
}

END_NAMESPACE_YM_YMSL