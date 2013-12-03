#ifndef SRC_DTPG_DTPGCMD_H
#define SRC_DTPG_DTPGCMD_H

/// @file src/atpg_tclsh/DtpgCmd.h
/// @brief DtpgCmd のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2010, 2012 Yusuke Matsunaga
/// All rights reserved.


#include "AtpgCmd.h"


BEGIN_NAMESPACE_YM_SATPG

//////////////////////////////////////////////////////////////////////
// テストパタン生成を行うコマンド
//////////////////////////////////////////////////////////////////////
class DtpgCmd :
  public AtpgCmd
{
public:

  /// @brief コンストラクタ
  DtpgCmd(AtpgMgr* mgr);

  /// @brief デストラクタ
  virtual
  ~DtpgCmd();


protected:

  /// @brief コマンド処理関数
  virtual
  int
  cmd_proc(TclObjVector& objv);


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // verbose オプションの解析用オブジェクト
  TclPoptInt* mPoptVerbose;

  // sat モードオプションの解析用オブジェクト
  TclPopt* mPoptSat;

  // satrec モードオプションの解析用オブジェクト
  TclPopt* mPoptSatRec;

  // minisat モードオプションの解析用オブジェクト
  TclPopt* mPoptMiniSat;

  // print_stats オプションの解析用オブジェクト
  TclPopt* mPoptPrintStats;

  // single オプションの解析用オブジェクト
  TclPopt* mPoptSingle;

  // dual オプションの解析用オブジェクト
  TclPopt* mPoptDual;

  // node オプションの解析用オブジェクト
  TclPopt* mPoptNode;

  // ffr オプションの解析用オブジェクト
  TclPopt* mPoptFFR;

  // ffr2 オプションの解析用オブジェクト
  TclPoptInt* mPoptFFR2;

  // mffc オプションの解析用オブジェクト
  TclPopt* mPoptMFFC;

  // all オプションの解析用オブジェクト
  TclPopt* mPoptAll;

  // po オプションの解析用オブジェクト
  TclPopt* mPoptPo;

  // rpo オプションの解析用オブジェクト
  TclPopt* mPoptRpo;

  // skip オプションの解析用オブジェクト
  TclPoptInt* mPoptSkip;

  // X抽出オプションの解析用オブジェクト
  TclPoptInt* mPoptX;

  // drop オプションの解析用オブジェクト
  TclPopt* mPoptDrop;

  // verify オプションの解析用オブジェクト
  TclPopt* mPoptVerify;

  // timer オプションの解析用オブジェクト
  TclPopt* mPoptTimer;

};

END_NAMESPACE_YM_SATPG

#endif // SRC_DTPG_DTPGCMD_H