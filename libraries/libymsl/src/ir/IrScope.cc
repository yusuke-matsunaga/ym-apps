
/// @file IrScopeExpr.cc
/// @brief IrScopeExpr の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2014 Yusuke Matsunaga
/// All rights reserved.


#include "IrScopeExpr.h"
#include "IrHandleImpl.h"


BEGIN_NAMESPACE_YM_YMSL

//////////////////////////////////////////////////////////////////////
// クラス IrScopeExpr
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] parent 親のスコープ
// @param[in] name 名前
IrScopeExpr::IrScopeExpr(IrScopeExpr* parent,
		     ShString name) :
  mParent(parent),
  mName(name),
  mHashSize(0),
  mHashTable(NULL),
  mHashNum(0)
{
  alloc_table(16);
}

// @brief デストラクタ
IrScopeExpr::~IrScopeExpr()
{
  for (ymuint i = 0; i < mHashSize; ++ i) {
    for (IrHandle* h = mHashTable[i]; h; ) {
      IrHandle* n = h->mLink;
      delete h;
      h = n;
    }
  }
  delete [] mHashTable;
}

// @brief 自身の名前を返す．
//
// ShString() (空文字列)の場合もある．
ShString
IrScopeExpr::name() const
{
  return mName;
}

// @brief ラベルを追加する．
// @param[in] item 追加する要素
void
IrScopeExpr::add_label(Label* item)
{
  IrHandle* handle = new LabelHandle(item);
  put(handle);
}

// @brief 関数を追加する．
// @param[in] item 追加する要素
void
IrScopeExpr::add_function(Function* item)
{
  IrHandle* handle = new FuncHandle(item);
  put(handle);
}

// @brief 変数を追加する．
// @param[in] item 追加する要素
void
IrScopeExpr::add_var(Var* item)
{
  IrHandle* handle = new VarHandle(item);
  put(handle);
}

// @brief 型を追加する．
// @param[in] item 追加する要素
void
IrScopeExpr::add_type(Type* item)
{
  IrHandle* handle = new TypeHandle(item);
  put(handle);
}

// @brief スコープを追加する．
// @param[in] item 追加する要素
void
IrScopeExpr::add_scope(IrScopeExpr* item)
{
  IrHandle* handle = new IrScopeExprHandle(item);
  put(handle);
}

// @brief ハッシュ表を確保する．
void
IrScopeExpr::alloc_table(ymuint req_size)
{
  ymuint old_size = mHashSize;
  IrHandle** old_table = mHashTable;
  mHashSize = req_size;
  mNextLimit = static_cast<ymuint>(mHashSize * 1.8);
  mHashTable = new IrHandle*[mHashSize];
  for (ymuint i = 0; i < mHashSize; ++ i) {
    mHashTable[i] = NULL;
  }
  for (ymuint i = 0; i < old_size; ++ i) {
    for (IrHandle* handle = old_table[i];
	 handle != NULL; ) {
      IrHandle* next = handle->mLink;
      _put(handle);
      handle = next;
    }
  }
  delete [] old_table;
}

// @brief 名前からハンドルを探す．
// @param[in] name 名前
IrHandle*
IrScopeExpr::find(ShString name) const
{
  ymuint pos = name.hash() % mHashSize;
  for (IrHandle* handle = mHashTable[pos];
       handle != NULL; handle = handle->mLink) {
    if ( handle->name() == name ) {
      return handle;
    }
  }
  if ( mParent != NULL ) {
    return mParent->find(name);
  }
  return NULL;
}

// @brief ハンドルを登録する．
void
IrScopeExpr::put(IrHandle* handle)
{
  if ( mHashNum >= mNextLimit ) {
    alloc_table(mHashSize << 1);
  }

  _put(handle);
}

// @brief ハンドルを登録する．
//
// こちらはサイズチェックなし
void
IrScopeExpr::_put(IrHandle* handle)
{
  ymuint pos = handle->name().hash() % mHashSize;
  handle->mLink = mHashTable[pos];
  mHashTable[pos] = handle;
  ++ mHashNum;
}

#if 0
// @brief ステートメント数を返す．
ymuint
IrScopeExpr::statement_num() const
{
  return mStatementList.size();
}

// @brief ステートメントを返す．
// @param[in] pos 位置 ( 0 <= pos < statement_num() )
AstStatement*
IrScopeExpr::statement(ymuint pos) const
{
  ASSERT_COND( pos < statement_num() );
  return mStatementList[pos];
}

// @brief 命令コードのサイズを計算する．
ymuint
IrScopeExpr::calc_size() const
{
  ymuint size = 0;
  for (vector<AstStatement*>::const_iterator p = mStatementList.begin();
       p != mStatementList.end(); ++ p) {
    AstStatement* stmt = *p;
    size += stmt->calc_size();
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
IrScopeExpr::compile(IrDriver& driver,
		  IrCodeList& code_list,
		  Ir_INT& addr)
{
  for (vector<AstStatement*>::const_iterator p = mStatementList.begin();
       p != mStatementList.end(); ++ p) {
    AstStatement* stmt = *p;
    stmt->compile(driver, code_list, addr);
  }
}

// @brief 内容を表示する．(デバッグ用)
// @param[in] s 出力ストリーム
// @param[in] indent インデントレベル
void
IrScopeExpr::print(ostream& s,
		ymuint indent) const
{
  ymuint n = mStatementList.size();
  for (ymuint i = 0; i < n; ++ i) {
    mStatementList[i]->print(s, indent);
  }
}
#endif

END_NAMESPACE_YM_YMSL
