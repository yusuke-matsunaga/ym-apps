﻿
/// @file recordcut.cc
/// @brief カットを記録するプログラム
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2012 Yusuke Matsunaga
/// All rights reserved.


#if HAVE_POPT
#include <popt.h>
#else
#error "<popt.h> not found."
#endif

#include "YmNetworks/BdnMgr.h"
#include "YmNetworks/BdnBlifReader.h"
#include "YmNetworks/BdnIscas89Reader.h"
#include "YmNetworks/BdnNode.h"

#include "TopDown.h"
#include "TopDown2.h"
#include "FuncMgr.h"
#include "FuncRec.h"

#include "GbmSolver.h"
#include "Lut443Match.h"
#include "Lut444Match.h"

#include "YmUtils/MsgMgr.h"
#include "YmUtils/MsgHandler.h"

#include "YmUtils/StopWatch.h"

#include "YmUtils/FileIDO.h"
#include "YmUtils/FileODO.h"
#include "YmUtils/StopWatch.h"
#include "YmUtils/RandGen.h"

BEGIN_NAMESPACE_YM

bool verbose = false;

void
rec_func(FuncMgr& func_mgr,
	 const string& filename,
	 bool blif,
	 bool iscas89,
	 bool lut444,
	 ymuint min_cut_size,
	 ymuint max_cut_size,
	 const string& method,
	 ymuint nrand)
{
  MsgHandler* msg_handler = new StreamMsgHandler(&cerr);
  MsgMgr::reg_handler(msg_handler);

  BdnMgr network;

  if ( blif ) {
    BdnBlifReader read;
    if ( !read(filename, network) ) {
      cerr << "Error in reading " << filename << endl;
      return;
    }
  }
  else {
    BdnIscas89Reader read;
    if ( !read(filename, network) ) {
      cerr << "Error in reading " << filename << endl;
      return;
    }
  }

  FuncRec op(func_mgr);
#if 0
  TopDown2 enumcut;
#else
  TopDown enumcut;
#endif

  op.set_min_size(min_cut_size);
  op.set_debug_level(0);

  enumcut(network, max_cut_size, &op);

  vector<TvFunc> func_list;
  func_mgr.func_list(func_list);

  cout << "Total " << setw(12) << func_list.size() << " functions" << endl;
  for (ymuint i = min_cut_size; i <= max_cut_size; ++ i) {
    vector<TvFunc> func_list;
    func_mgr.func_list(i, func_list);
    cout << "Total " << setw(12) << func_list.size() << " " << setw(2) << i << " input functions" << endl;
  }

  GbmSolver* solver = GbmSolver::new_solver(method);
  if ( solver == NULL ) {
    return;
  }

  StopWatch timer;
  timer.start();

  if ( lut444 ) {
    Lut444Match matcher;

    if ( nrand > 0 ) {
      RandGen rg;
      for (ymuint i = 0; i < nrand; ++ i) {
	ymuint pos = rg.ulong() % func_list.size();
	if ( verbose ) {
	  cout << "#" << i << endl;
	}
	matcher.match(func_list[pos], *solver);
      }
    }
    else {
      for (vector<TvFunc>::const_iterator p = func_list.begin();
	   p != func_list.end(); ++ p) {
	matcher.match(*p, *solver);
      }
    }

    ymuint t_num = 0;
    ymuint a0_num = 0;
    ymuint a0_loop = 0;
    ymuint a1_num = 0;
    ymuint a1_loop = 0;
    ymuint a2_num = 0;
    ymuint a2_loop = 0;
    ymuint a3_num = 0;
    ymuint a3_loop = 0;
    ymuint a4_num = 0;
    ymuint a4_loop = 0;
    ymuint b0_num = 0;
    ymuint b0_loop = 0;
    ymuint b1_num = 0;
    ymuint b1_loop = 0;
    ymuint b2_num = 0;
    ymuint b2_loop = 0;
    ymuint b3_num = 0;
    ymuint b3_loop = 0;
    ymuint c0_num = 0;
    ymuint c0_loop = 0;
    ymuint c1_num = 0;
    ymuint c1_loop = 0;
    ymuint c2_num = 0;
    ymuint c2_loop = 0;
    ymuint d0_num = 0;
    ymuint d0_loop = 0;
    ymuint d1_num = 0;
    ymuint d1_loop = 0;
    ymuint d2_num = 0;
    ymuint d2_loop = 0;
    ymuint e0_num = 0;
    ymuint e0_loop = 0;
    ymuint e1_num = 0;
    ymuint e1_loop = 0;
    ymuint e2_num = 0;
    ymuint e2_loop = 0;
    ymuint f0_num = 0;
    ymuint f0_loop = 0;
    ymuint f1_num = 0;
    ymuint f1_loop = 0;
    ymuint g0_num = 0;
    ymuint g0_loop = 0;
    ymuint fail_num = 0;
    USTime ok_time;
    USTime ng_time;
    matcher.get_count(t_num,
		      a0_num,
		      a0_loop,
		      a1_num,
		      a1_loop,
		      a2_num,
		      a2_loop,
		      a3_num,
		      a3_loop,
		      a4_num,
		      a4_loop,
		      b0_num,
		      b0_loop,
		      b1_num,
		      b1_loop,
		      b2_num,
		      b2_loop,
		      b3_num,
		      b3_loop,
		      c0_num,
		      c0_loop,
		      c1_num,
		      c1_loop,
		      c2_num,
		      c2_loop,
		      d0_num,
		      d0_loop,
		      d1_num,
		      d1_loop,
		      d2_num,
		      d2_loop,
		      e0_num,
		      e0_loop,
		      e1_num,
		      e1_loop,
		      e2_num,
		      e2_loop,
		      f0_num,
		      f0_loop,
		      f1_num,
		      f1_loop,
		      g0_num,
		      g0_loop,
		      fail_num,
		      ok_time,
		      ng_time);

    cout << "Trivial: " << t_num << endl
	 << "A-0    : " << a0_num << ":  " << a0_loop << endl
	 << "A-1    : " << a1_num << ":  " << a1_loop << endl
	 << "A-2    : " << a2_num << ":  " << a2_loop << endl
	 << "A-3    : " << a3_num << ":  " << a3_loop << endl
	 << "A-4    : " << a4_num << ":  " << a4_loop << endl
	 << "B-0    : " << b0_num << ":  " << b0_loop << endl
	 << "B-1    : " << b1_num << ":  " << b1_loop << endl
	 << "B-2    : " << b2_num << ":  " << b2_loop << endl
	 << "B-3    : " << b3_num << ":  " << b3_loop << endl
	 << "C-0    : " << c0_num << ":  " << c0_loop << endl
	 << "C-1    : " << c1_num << ":  " << c1_loop << endl
	 << "C-2    : " << c2_num << ":  " << c2_loop << endl
	 << "D-0    : " << d0_num << ":  " << d0_loop << endl
	 << "D-1    : " << d1_num << ":  " << d1_loop << endl
	 << "D-2    : " << d2_num << ":  " << d2_loop << endl
	 << "E-0    : " << e0_num << ":  " << e0_loop << endl
	 << "E-1    : " << e1_num << ":  " << e1_loop << endl
	 << "E-2    : " << e2_num << ":  " << e2_loop << endl
	 << "F-0    : " << f0_num << ":  " << f0_loop << endl
	 << "F-1    : " << f1_num << ":  " << f1_loop << endl
	 << "G-0    : " << g0_num << ":  " << g0_loop << endl
	 << "Fail   : " << fail_num << endl
	 << "OK time: " << ok_time << endl
	 << "NG time: " << ng_time << endl;
  }
  else {
    Lut443Match matcher;

    if ( nrand > 0 ) {
      RandGen rg;
      for (ymuint i = 0; i < nrand; ++ i) {
	ymuint pos = rg.ulong() % func_list.size();
	if ( verbose ) {
	  cout << "#" << i << endl;
	}
	matcher.match(func_list[pos], *solver);
      }
    }
    else {
      for (vector<TvFunc>::const_iterator p = func_list.begin();
	   p != func_list.end(); ++ p) {
	matcher.match(*p, *solver);
      }
    }

    ymuint t_num = 0;
    ymuint a0_num = 0;
    ymuint a1_num = 0;
    ymuint a2_num = 0;
    ymuint a3_num = 0;
    ymuint b0_num = 0;
    ymuint b1_num = 0;
    ymuint b2_num = 0;
    ymuint c0_num = 0;
    ymuint c1_num = 0;
    ymuint a0_loop = 0;
    ymuint a1_loop = 0;
    ymuint a2_loop = 0;
    ymuint a3_loop = 0;
    ymuint b0_loop = 0;
    ymuint b1_loop = 0;
    ymuint b2_loop = 0;
    ymuint c0_loop = 0;
    ymuint c1_loop = 0;
    ymuint fail_num = 0;
    USTime ok_time;
    USTime ng_time;
    matcher.get_count(t_num,
		      a0_num,
		      a0_loop,
		      a1_num,
		      a1_loop,
		      a2_num,
		      a2_loop,
		      a3_num,
		      a3_loop,
		      b0_num,
		      b0_loop,
		      b1_num,
		      b1_loop,
		      b2_num,
		      b2_loop,
		      c0_num,
		      c0_loop,
		      c1_num,
		      c1_loop,
		      fail_num,
		      ok_time,
		      ng_time);

    cout << "Trivial: " << t_num << endl
	 << "A-0    : " << a0_num << ":     " << a0_loop << endl
	 << "A-1    : " << a1_num << ":     " << a1_loop << endl
	 << "A-2    : " << a2_num << ":     " << a2_loop << endl
	 << "A-3    : " << a3_num << ":     " << a3_loop << endl
	 << "B-0    : " << b0_num << ":     " << b0_loop << endl
	 << "B-1    : " << b1_num << ":     " << b1_loop << endl
	 << "B-2    : " << b2_num << ":     " << b2_loop << endl
	 << "C-0    : " << c0_num << ":     " << c0_loop << endl
	 << "C-1    : " << c1_num << ":     " << c1_loop << endl
	 << "Fail   : " << fail_num << endl
	 << "OK time: " << ok_time << endl
	 << "NG time: " << ng_time << endl;

  }

  timer.stop();
  cout << "Total CPUT time " << timer.time() << endl;

  delete solver;

#if 0
  const list<Cut*>& cut_list = cut_mgr.cut_list();
  cout << "Total " << cut_list.size() << " unique cuts" << endl;

  const char* datafile = "cuts.dump";
  {
    ofstream os;
    os.open(datafile, ios::binary);
    if ( !os ) {
      cerr << "Could not create " << datafile << endl;
      return;
    }
    ODOStream bos(os);

    cut_mgr.dump(bos);
  }

  CutMgr cut_mgr2;
  {
    ifstream is;
    is.open(datafile, ios::binary);
    if ( !is ) {
      cerr << "Could not open " << datafile << endl;
      return;
    }
    IDOStream bis(is);
    cut_mgr2.restore(bis);
  }
  cout << "cut_mgr2:: total " << cut_mgr2.cut_list().size() << " unique cuts" << endl;
#endif
}

void
dump_func(FuncMgr& func_mgr,
	  const char* filename)
{
  FileODO bo;
  if ( !bo.open(filename) ) {
    cerr << "Could not create " << filename << endl;
    return;
  }

  func_mgr.dump(bo);
}

void
restore_func(FuncMgr& func_mgr,
	     const char* filename)
{
  FileIDO bi;
  if ( !bi.open(filename) ) {
    cerr << "Could not create " << filename << endl;
    return;
  }

  func_mgr.restore(bi);
}

END_NAMESPACE_YM


int
main(int argc,
     const char** argv)
{
  using namespace std;
  using namespace nsYm;

  bool blif = false;
  bool iscas = false;
  bool lut444 = false;
  int max_cut_size = -1;
  int min_cut_size = -1;
  char* method_str = NULL;
  ymuint nrand = 0;

  // オプション解析用のデータ
  const struct poptOption options[] = {
    // long-option
    // short-option
    // argument type
    // variable address
    // option tag
    // docstr
    // argstr
    { "blif", '\0', POPT_ARG_NONE, NULL, 0x100,
      "blif mode", NULL },

    { "iscas89", '\0', POPT_ARG_NONE, NULL, 0x101,
      "iscas89 mode", NULL },

    { "max_cut_size", 'C', POPT_ARG_INT, &max_cut_size, 0,
      "specify maximum cut size", NULL },

    { "min_cut_size", 'c', POPT_ARG_INT, &min_cut_size, 0,
      "specify minimum cut size", NULL },

    { "method", 'm', POPT_ARG_STRING, &method_str, 0,
      "specify matcing method", NULL },

    { "randam_sample", 'r', POPT_ARG_INT, &nrand, 0,
      "do randam smpling", "# of samples" },

    { "lut444", 0, POPT_ARG_NONE, NULL, 0x110,
      "LUT444", NULL },

    { "verbose", 'v', POPT_ARG_NONE, NULL, 0x200,
      "set verbose flag", NULL },

    POPT_AUTOHELP

    { NULL, '\0', 0, NULL, 0, NULL, NULL }
  };

  // オプション解析用のコンテキストを生成する．
  poptContext popt_context = poptGetContext(NULL, argc, argv, options, 0);
  poptSetOtherOptionHelp(popt_context, "[OPTIONS]* <file-name> ...");

  // オプション解析行う．
  for ( ; ; ) {
    int rc = poptGetNextOpt(popt_context);
    if ( rc == -1 ) {
      break;
    }
    if ( rc < -1 ) {
      // エラーが起きた．
      fprintf(stderr, "%s: %s\n",
	      poptBadOption(popt_context, POPT_BADOPTION_NOALIAS),
	      poptStrerror(rc));
      return 1;
    }
    if ( rc == 0x100 ) {
      blif = true;
    }
    else if ( rc == 0x101 ) {
      iscas = true;
    }
    else if ( rc == 0x200 ) {
      verbose = true;
    }
    else if ( rc == 0x110 ) {
      lut444 = true;
    }
  }

  if ( max_cut_size == -1 ) {
    if ( min_cut_size == -1 ) {
      max_cut_size = 4;
      min_cut_size = 4;
      cerr << "Both min_cut_size and max_cut_size are not specified. '4' is assumed" << endl;
    }
    else {
      max_cut_size = min_cut_size;
    }
  }
  else {
    if ( min_cut_size == -1 ) {
      min_cut_size = max_cut_size;
    }
  }

  string method;
  if ( method_str != NULL ) {
    method = method_str;
  }

  if ( !blif && !iscas ) {
    blif = true;
  }

  // 残りの引数はファイル名とみなす．
  const char* str = poptGetArg(popt_context);
  if ( str == NULL ) {
    fprintf(stderr, "No filename.\n");
    return 2;
  }

  FuncMgr func_mgr;

  string filename(str);
  rec_func(func_mgr, filename, blif, iscas, lut444, min_cut_size, max_cut_size, method, nrand);

  return 0;
}
