﻿#ifndef SRC_PT_PTXOROP_H
#define SRC_PT_PTXOROP_H

/// @file src/pt/PtXorOp.h
/// @brief PtXorOp のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2011 Yusuke Matsunaga
/// All rights reserved.


#include "PtBinaryOp.h"


BEGIN_NAMESPACE_YM_BB

//////////////////////////////////////////////////////////////////////
/// @class PtXorOp PtXorOp.h "PtXorOp.h"
/// @brief 加算を表すノード
//////////////////////////////////////////////////////////////////////
class PtXorOp :
  public PtBinaryOp
{
public:

  /// @brief コンストラクタ
  /// @param[in] file_region ファイル上の位置
  /// @param[in] opr1, opr2 オペランド
  PtXorOp(const FileRegion& file_region,
	  PtNode* opr1,
	  PtNode* opr2);

  /// @brief デストラクタ
  virtual
  ~PtXorOp();


public:
  //////////////////////////////////////////////////////////////////////
  // PtNode の仮想関数
  //////////////////////////////////////////////////////////////////////

  /// @brief 型を返す．
  virtual
  tType
  type() const;

  /// @brief 内容を表す文字列を返す．
  virtual
  string
  decompile() const;

  /// @brief 対応した AIG を作る．
  virtual
  void
  gen_aig(AigMgr& aigmgr,
	  const vector<Aig>& bvar_array,
	  ymuint bw,
	  vector<Aig>& out_array);

};

END_NAMESPACE_YM_BB

#endif // SRC_PT_PTXOROP_H
