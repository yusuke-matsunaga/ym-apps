﻿
/// @file npngen.cc
/// @brief npngen のソースファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2012 Yusuke Matsunaga
/// All rights reserved.


#include "YmUtils/PermGen.h"


BEGIN_NAMESPACE_YM

void
npngen(ymuint ni)
{
  ASSERT_COND( ni == 3 || ni == 4 );


  const char* comma0 = "";
  for (PermGen pg(ni, ni); !pg.is_end(); ++ pg) {
    ymuint8 np = 1U << (ni + 1);
    for (ymuint8 pols = 0U; pols < np; ++ pols) {
      if ( comma0[0] != '\0' ) {
	cout << comma0 << endl;
      }
      cout << "  {";
      const char* comma = "";
      for (ymuint i = 0; i < ni; ++ i) {
	cout << comma << pg(i);
	comma = ", ";
      }
      cout << comma << static_cast<ymuint>(pols) << "}";
      comma0 = ",";
    }
  }
  cout << endl;
}

void
usage(char* progname)
{
  cerr << "USAGE: " << progname << " number-of-inputs(3 or 4)" << endl;
}

END_NAMESPACE_YM

int
main(int argc,
     char** argv)
{
  using namespace std;
  using namespace nsYm;

  if ( argc != 2 ) {
    usage(argv[0]);
    exit(1);
  }

  ymuint ni = atoi(argv[1]);

  if ( ni != 3 && ni != 4 ) {
    usage(argv[0]);
    exit(2);
  }

  npngen(ni);

  return 0;
}
