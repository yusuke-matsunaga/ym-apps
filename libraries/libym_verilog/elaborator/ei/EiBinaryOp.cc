
/// @file libym_verilog/elb_impl/EiBinaryOp.cc
/// @brief EiBinaryOp の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// $Id: EiBinaryOp.cc 2507 2009-10-17 16:24:02Z matsunaga $
///
/// Copyright (C) 2005-2010 Yusuke Matsunaga
/// All rights reserved.


#include "EiFactory.h"
#include "EiBinaryOp.h"

#include "ym_verilog/VlOpType.h"
#include "ym_verilog/BitVector.h"


BEGIN_NAMESPACE_YM_VERILOG

//////////////////////////////////////////////////////////////////////
// EiFactory の生成関数
//////////////////////////////////////////////////////////////////////

// @brief 2項演算子を生成する．
// @param[in] pt_expr パース木の定義要素
// @param[in] op_type 演算子のタイプ
// @param[in] opr0 オペランド
// @param[in] opr1 オペランド
ElbExpr*
EiFactory::new_BinaryOp(const PtExpr* pt_expr,
			const VlOpType& op_type,
			ElbExpr* opr0,
			ElbExpr* opr1)
{
  ElbExpr* expr = NULL;
  void* p;
  switch( op_type.val() ) {
  case vpiBitAndOp:
  case vpiBitOrOp:
  case vpiBitXNorOp:
  case vpiBitXorOp:
    p = mAlloc.get_memory(sizeof(EiBinaryBitOp));
    expr = new (p) EiBinaryBitOp(pt_expr, opr0, opr1);
    break;

  case vpiAddOp:
  case vpiSubOp:
  case vpiMultOp:
  case vpiDivOp:
  case vpiModOp:
    p = mAlloc.get_memory(sizeof(EiBinaryArithOp));
    expr = new (p) EiBinaryArithOp(pt_expr, opr0, opr1);
    break;

  case vpiPowerOp:
    p = mAlloc.get_memory(sizeof(EiPowerOp));
    expr = new (p) EiPowerOp(pt_expr, opr0, opr1);
    break;

  case vpiLShiftOp:
  case vpiRShiftOp:
  case vpiArithLShiftOp:
  case vpiArithRShiftOp:
    p = mAlloc.get_memory(sizeof(EiShiftOp));
    expr = new (p) EiShiftOp(pt_expr, opr0, opr1);
    break;

  case vpiLogAndOp:
  case vpiLogOrOp:
    p = mAlloc.get_memory(sizeof(EiBinaryLogOp));
    expr = new (p) EiBinaryLogOp(pt_expr, opr0, opr1);
    break;

  case vpiCaseEqOp:
  case vpiCaseNeqOp:
  case vpiEqOp:
  case vpiNeqOp:
  case vpiGeOp:
  case vpiGtOp:
  case vpiLeOp:
  case vpiLtOp:
    p = mAlloc.get_memory(sizeof(EiCompareOp));
    expr = new (p) EiCompareOp(pt_expr, opr0, opr1);
    break;

  default:
    assert_not_reached(__FILE__, __LINE__);
    break;
  }
  return expr;
}


//////////////////////////////////////////////////////////////////////
// クラス EIBinaryOp
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] pt_expr パース木の定義要素
// @param[in] opr1 オペランド1
// @param[in] opr2 オペランド2
EiBinaryOp::EiBinaryOp(const PtExpr* pt_expr,
		       ElbExpr* opr1,
		       ElbExpr* opr2) :
  EiOperation(pt_expr)
{
  mOpr[0] = opr1;
  mOpr[1] = opr2;
}

// @brief デストラクタ
EiBinaryOp::~EiBinaryOp()
{
}

// @brief 定数の時 true を返す．
// @note オペランドが定数ならこの式も定数となる．
bool
EiBinaryOp::is_const() const
{
  return operand1()->is_const() && operand2()->is_const();
}

// @brief オペランド数を返す．
ymuint
EiBinaryOp::operand_num() const
{
  return 2;
}

// @brief オペランドを返す．
// @param[in] pos 位置番号
ElbExpr*
EiBinaryOp::_operand(ymuint pos) const
{
  return mOpr[pos];
}


//////////////////////////////////////////////////////////////////////
// クラス EiCompareOp
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] pt_expr パース木の定義要素
// @param[in] opr1 オペランド1
// @param[in] opr2 オペランド2
EiCompareOp::EiCompareOp(const PtExpr* pt_expr,
			 ElbExpr* opr1,
			 ElbExpr* opr2) :
  EiBinaryOp(pt_expr, opr1, opr2)
{
  // 比較演算は大きい方の型を用いる．
  VlValueType type1 = operand1()->value_type();
  VlValueType type2 = operand2()->value_type();

  mOprType = calc_type(type1, type2);

  operand1()->set_reqsize(mOprType);
  operand2()->set_reqsize(mOprType);
}

// @brief デストラクタ
EiCompareOp::~EiCompareOp()
{
}

// @brief 式のタイプを返す．
VlValueType
EiCompareOp::value_type() const
{
  // 常に1ビット符号なし
  return VlValueType(false, true, 1);
}

// @brief オペランドに要求されるデータ型を返す．
// @param[in] pos 位置番号 ( 0 <= pos < operand_num() )
// @note kVpiOperation の時，意味を持つ．
// @note それ以外では kVpiValueNone を返す．
// 通常はオペランドの式の value_type() に一致するが，
// その式が self-typed の場合には異なることもある．
VlValueType
EiCompareOp::operand_type(ymuint pos) const
{
  return mOprType;
}

// @brief 要求される式の型を計算してセットする．
// @param[in] type 要求される式の型
// @note 必要であればオペランドに対して再帰的に処理を行なう．
void
EiCompareOp::set_reqsize(const VlValueType& type)
{
  // なにもしない．
}


//////////////////////////////////////////////////////////////////////
// クラス EiBinaryLogOp
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] pt_expr パース木の定義要素
// @param[in] opr1 オペランド1
// @param[in] opr2 オペランド2
EiBinaryLogOp::EiBinaryLogOp(const PtExpr* pt_expr,
			     ElbExpr* opr1,
			     ElbExpr* opr2) :
  EiBinaryOp(pt_expr, opr1, opr2)
{
  // 論理演算の場合はオペランドも1ビットスカラーのはずだが
  // 仕様書には max(L(i), L(j)) なんて書いてある．
  // これは == などと一緒に表記しているからだと思う．
  // 実際には self determined で計算しておいて
  // 評価時に true/false/X に分類する．
  opr1->set_selfsize();
  opr2->set_selfsize();
}

// @brief デストラクタ
EiBinaryLogOp::~EiBinaryLogOp()
{
}

// @brief 式のタイプを返す．
VlValueType
EiBinaryLogOp::value_type() const
{
  // 常に1ビット符号なし
  return VlValueType(false, true, 1);
}

// @brief オペランドに要求されるデータ型を返す．
// @param[in] pos 位置番号 ( 0 <= pos < operand_num() )
// @note kVpiOperation の時，意味を持つ．
// @note それ以外では kVpiValueNone を返す．
// 通常はオペランドの式の value_type() に一致するが，
// その式が self-typed の場合には異なることもある．
VlValueType
EiBinaryLogOp::operand_type(ymuint pos) const
{
  // 常に1ビット符号なし
  return VlValueType(false, true, 1);
}

// @brief 要求される式の型を計算してセットする．
// @param[in] type 要求される式の型
// @note 必要であればオペランドに対して再帰的に処理を行なう．
void
EiBinaryLogOp::set_reqsize(const VlValueType& type)
{
  // なにもしない．
}


//////////////////////////////////////////////////////////////////////
// クラス EiBinaryBitOp
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] pt_expr パース木の定義要素
// @param[in] opr1 オペランド1
// @param[in] opr2 オペランド2
EiBinaryBitOp::EiBinaryBitOp(const PtExpr* pt_expr,
			     ElbExpr* opr1,
			     ElbExpr* opr2) :
  EiBinaryOp(pt_expr, opr1, opr2)
{
  // オペランドのサイズの大きい方で決まる．
  VlValueType type1 = operand1()->value_type();
  VlValueType type2 = operand2()->value_type();

  mType = calc_type(type1, type2);

  assert_cond( !mType.is_real_type(), __FILE__, __LINE__);
}

// @brief デストラクタ
EiBinaryBitOp::~EiBinaryBitOp()
{
}

// @brief 式のタイプを返す．
VlValueType
EiBinaryBitOp::value_type() const
{
  return mType;
}

// @brief オペランドに要求されるデータ型を返す．
// @param[in] pos 位置番号 ( 0 <= pos < operand_num() )
// @note kVpiOperation の時，意味を持つ．
// @note それ以外では kVpiValueNone を返す．
// 通常はオペランドの式の value_type() に一致するが，
// その式が self-typed の場合には異なることもある．
VlValueType
EiBinaryBitOp::operand_type(ymuint pos) const
{
  return mType;
}

// @brief 要求される式の型を計算してセットする．
// @param[in] type 要求される式の型
// @note 必要であればオペランドに対して再帰的に処理を行なう．
void
EiBinaryBitOp::set_reqsize(const VlValueType& type)
{
  mType = update_size(mType, type);
  operand1()->set_reqsize(mType);
  operand2()->set_reqsize(mType);
}


//////////////////////////////////////////////////////////////////////
// クラス EiBinaryArithOp
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] pt_expr パース木の定義要素
// @param[in] opr1 オペランド1
// @param[in] opr2 オペランド2
EiBinaryArithOp::EiBinaryArithOp(const PtExpr* pt_expr,
				 ElbExpr* opr1,
				 ElbExpr* opr2) :
  EiBinaryOp(pt_expr, opr1, opr2)
{
  // オペランドのサイズの大きい方で決まる．
  VlValueType type1 = operand1()->value_type();
  VlValueType type2 = operand2()->value_type();

  mType = calc_type(type1, type2);
}

// @brief デストラクタ
EiBinaryArithOp::~EiBinaryArithOp()
{
}

// @brief 式のタイプを返す．
VlValueType
EiBinaryArithOp::value_type() const
{
  return mType;
}

// @brief オペランドに要求されるデータ型を返す．
// @param[in] pos 位置番号 ( 0 <= pos < operand_num() )
// @note kVpiOperation の時，意味を持つ．
// @note それ以外では kVpiValueNone を返す．
// 通常はオペランドの式の value_type() に一致するが，
// その式が self-typed の場合には異なることもある．
VlValueType
EiBinaryArithOp::operand_type(ymuint pos) const
{
  return mType;
}

// @brief 要求される式の型を計算してセットする．
// @param[in] type 要求される式の型
// @note 必要であればオペランドに対して再帰的に処理を行なう．
void
EiBinaryArithOp::set_reqsize(const VlValueType& type)
{
  mType = update_size(mType, type);
  operand1()->set_reqsize(mType);
  operand2()->set_reqsize(mType);
}


//////////////////////////////////////////////////////////////////////
// クラス EiPowerOp
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] pt_expr パース木の定義要素
// @param[in] opr1 オペランド1
// @param[in] opr2 オペランド2
EiPowerOp::EiPowerOp(const PtExpr* pt_expr,
		     ElbExpr* opr1,
		     ElbExpr* opr2) :
  EiBinaryOp(pt_expr, opr1, opr2)
{
  // 巾乗演算の場合, どちらかのオペランドが real, integer, signed
  // なら結果は real, どちらも unsigned の時のみ unsigned となる．
  VlValueType type1 = operand1()->value_type();
  VlValueType type2 = operand2()->value_type();
  mType = calc_type2(type1, type2);

  // ただし opr2 は self-determined
  operand2()->set_reqsize(type2);
}

// @brief デストラクタ
EiPowerOp::~EiPowerOp()
{
}

// @brief 式のタイプを返す．
VlValueType
EiPowerOp::value_type() const
{
  return mType;
}

// @brief オペランドに要求されるデータ型を返す．
// @param[in] pos 位置番号 ( 0 <= pos < operand_num() )
// @note kVpiOperation の時，意味を持つ．
// @note それ以外では kVpiValueNone を返す．
// 通常はオペランドの式の value_type() に一致するが，
// その式が self-typed の場合には異なることもある．
VlValueType
EiPowerOp::operand_type(ymuint pos) const
{
  if ( pos == 0 ) {
    return mType;
  }
  else {
    return operand2()->value_type();
  }
}

// @brief 要求される式の型を計算してセットする．
// @param[in] type 要求される式の型
// @note 必要であればオペランドに対して再帰的に処理を行なう．
void
EiPowerOp::set_reqsize(const VlValueType& type)
{
  mType = update_size(mType, type);

  // 第1オペランドは type をそのまま伝える．
  operand1()->set_reqsize(mType);

  // 第2オペランドは不変
}


//////////////////////////////////////////////////////////////////////
// クラス EiShiftOp
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] pt_expr パース木の定義要素
// @param[in] opr1 オペランド1
// @param[in] opr2 オペランド2
EiShiftOp::EiShiftOp(const PtExpr* pt_expr,
		     ElbExpr* opr1,
		     ElbExpr* opr2) :
  EiBinaryOp(pt_expr, opr1, opr2)
{
  // シフト演算子は第1オペランドの型とサイズをそのまま引き継ぐ
  mType = opr1->value_type();
  assert_cond( !mType.is_real_type(), __FILE__, __LINE__);

  assert_cond( !opr2->value_type().is_real_type(), __FILE__, __LINE__);

  // 第2オペランドのサイズは self determined
  opr2->set_selfsize();
}

// @brief デストラクタ
EiShiftOp::~EiShiftOp()
{
}

// @brief 式のタイプを返す．
VlValueType
EiShiftOp::value_type() const
{
  return mType;
}

// @brief オペランドに要求されるデータ型を返す．
// @param[in] pos 位置番号 ( 0 <= pos < operand_num() )
// @note kVpiOperation の時，意味を持つ．
// @note それ以外では kVpiValueNone を返す．
// 通常はオペランドの式の value_type() に一致するが，
// その式が self-typed の場合には異なることもある．
VlValueType
EiShiftOp::operand_type(ymuint pos) const
{
  if ( pos == 0 ) {
    mType;
  }
  else {
    operand2()->value_type();
  }
}

// @brief 要求される式の型を計算してセットする．
// @param[in] type 要求される式の型
// @note 必要であればオペランドに対して再帰的に処理を行なう．
void
EiShiftOp::set_reqsize(const VlValueType& type)
{
  mType = update_size(mType, type);

  // 第1オペランドは type をそのまま伝える．
  operand1()->set_reqsize(mType);

  // 第2オペランドは不変
}

END_NAMESPACE_YM_VERILOG
