
/// @file lutmap/src/lutmap.cc
/// @brief lutmap のメイン関数
/// @author Yusuke Matsunaga (松永 裕介)
///
/// $Id: bliftest.cc 2507 2009-10-17 16:24:02Z matsunaga $
///
/// Copyright (C) 2005-2010 Yusuke Matsunaga
/// All rights reserved.


#include "ym_lutmap/lutmap_nsdef.h"
#include "ym_lutmap/SbjGraph.h"
#include "ym_lutmap/LnGraph.h"
#include "ym_bnet/BNetwork.h"
#include "ym_bnet/BNetBlifReader.h"
#include "ym_bnet/BNetDecomp.h"
#include "ym_utils/MsgHandler.h"


int
main(int argc,
     char** argv)
{
  using namespace std;
  using namespace nsYm;
  
  if ( argc != 2 ) {
    cerr << "USAGE : " << argv[0] << " blif-file" << endl;
    return 2;
  }
  string filename = argv[1];
  
  try {
    MsgHandler* msg_handler = new StreamMsgHandler(&cerr);
    BNetBlifReader reader;

    reader.add_msg_handler(msg_handler);
    
    BNetwork network;

    if ( !reader.read(filename, network) ) {
      cerr << "Error in reading " << filename << endl;
      return 4;
    }

    // 2入力ノードに分解
    BNetDecomp decomp;
    decomp(network, 2);

    // SbjGraph に変換
    SbjGraph sbj_network;
    bnet2sbj(network, sbj_network, cerr);

    // LUT にマッピング
    LnGraph lut_network;
    ymuint lut_num;
    ymuint depth;
    delay_map(sbj_network,
	      4,            // LUT の入力数
	      0,            // スラック
	      3,            // モード(0から3の値)
	      lut_network,
	      lut_num,
	      depth);

    // とりあえず内容を出力する．
    lut_network.dump(cout);
    
  }
  catch ( AssertError x) {
    cout << x << endl;
  }

  return 0;
}
