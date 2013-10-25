
/// @file NameObjImpl.cc
/// @brief NameObjImpl の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2013 Yusuke Matsunaga
/// All rights reserved.


#include "NameObjImpl.h"


BEGIN_NAMESPACE_YM_SMTLIBV2

//////////////////////////////////////////////////////////////////////
// クラス NameObjImpl
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] name_id 名前を表す識別子
NameObjImpl::NameObjImpl(const SmtId* name_id) :
  mName(name_id),
  mLink(NULL)
{
}

// @brief デストラクタ
NameObjImpl::~NameObjImpl()
{
}

// @brief 名前を表す識別子を返す．
const SmtId*
NameObjImpl::name() const
{
  return mName;
}

// @brief SmtFun を持っているとき true を返す．
bool
NameObjImpl::is_fun() const
{
  return false;
}

// @brief SmtVar を持っているとき true を返す．
bool
NameObjImpl::is_var() const
{
  return false;
}

// @brief SmtFun を返す．
// @param[in] input_sort_list 入力の型のリスト
// @note input_sort_list に合致する関数がない場合 NULL を返す．
const SmtFun*
NameObjImpl::fun(const vector<const SmtSort*>& input_sort_list) const
{
  assert_not_reached(__FILE__, __LINE__);
  return NULL;
}

// @brief SmtVar を返す．
const SmtVar*
NameObjImpl::var() const
{
  assert_not_reached(__FILE__, __LINE__);
  return NULL;
}


//////////////////////////////////////////////////////////////////////
// クラス FunObj
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] name_id 名前を表す識別子
// @param[in] fun 関数
FunObj::FunObj(const SmtId* name_id,
	       const SmtFun* fun) :
  NameObjImpl(name_id),
  mFun(fun)
{
}

// @brief デストラクタ
FunObj::~FunObj()
{
}

// @brief SmtFun を持っているとき true を返す．
bool
FunObj::is_fun() const
{
  return true;
}

// @brief SmtFun を返す．
// @param[in] input_sort_list 入力の型のリスト
// @note input_sort_list に合致する関数がない場合 NULL を返す．
const SmtFun*
FunObj::fun(const vector<const SmtSort*>& input_sort_list) const
{
  return mFun;
}


//////////////////////////////////////////////////////////////////////
// クラス VarObj
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] name_id 名前を表す識別子
// @param[in] var 変数
VarObj::VarObj(const SmtId* name_id,
	       const SmtVar* var) :
  NameObjImpl(name_id),
  mVar(var)
{
}

// @brief デストラクタ
VarObj::~VarObj()
{
}

// @brief SmtVar を持っているとき true を返す．
bool
VarObj::is_var() const
{
  return true;
}

// @brief SmtVar を返す．
const SmtVar*
VarObj::var() const
{
  return mVar;
}

END_NAMESPACE_YM_SMTLIBV2
