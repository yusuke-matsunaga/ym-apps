#ifndef ASTIF_H
#define ASTIF_H

/// @file AstIf.h
/// @brief AstIf のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2014 Yusuke Matsunaga
/// All rights reserved.


#include "AstStatement.h"


BEGIN_NAMESPACE_YM_YMSL

//////////////////////////////////////////////////////////////////////
/// @class AstIf AstIf.h "AstIf.h"
/// @brief if 文を表す Ast
//////////////////////////////////////////////////////////////////////
class AstIf :
  public AstStatement
{
public:

  /// @brief コンストラクタ
  /// @param[in] if_list IfBlock のリスト
  /// @param[in] loc ファイル位置
  AstIf(AstIfBlock* if_list,
	const FileRegion& loc);

  /// @brief デストラクタ
  virtual
  ~AstIf();


public:
  //////////////////////////////////////////////////////////////////////
  // 外部インターフェイス
  //////////////////////////////////////////////////////////////////////

  /// @brief 命令コードのサイズを計算する．
  virtual
  ymuint
  calc_size();

  /// @brief 命令コードを生成する．
  /// @param[in] driver ドライバ
  /// @param[in] code_list 命令コードの格納先
  /// @param[inout] addr 命令コードの現在のアドレス
  ///
  /// addr の値は更新される．
  virtual
  void
  compile(YmslDriver& driver,
	  YmslCodeList& code_list,
	  Ymsl_INT& addr);

  /// @brief 内容を表示する．(デバッグ用)
  /// @param[in] s 出力ストリーム
  /// @param[in] indent インデントレベル
  virtual
  void
  print(ostream& s,
	ymuint indent = 0) const;


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // if ブロックのリスト
  vector<AstIfBlock*> mIfBlockList;

};

END_NAMESPACE_YM_YMSL


#endif // ASTIF_H
