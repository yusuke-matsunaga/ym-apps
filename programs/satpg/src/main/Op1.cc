
/// @file atpg/src/main/Op1.cc
/// @brief Op1 の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2010, 2012 Yusuke Matsunaga
/// All rights reserved.


#include "Op1.h"
#include "SaFault.h"
#include "FaultMgr.h"
#include "TvMgr.h"
#include "Fsim.h"
#include "TestVector.h"


BEGIN_NAMESPACE_YM_SATPG

//////////////////////////////////////////////////////////////////////
// クラス Op1
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
Op1::Op1(FaultMgr& fmgr,
	 TvMgr& tvmgr,
	 vector<TestVector*>& tv_list,
	 Fsim& fsim3,
	 bool verify) :
  mFaultMgr(fmgr),
  mTvMgr(tvmgr),
  mTvList(tv_list),
  mFsim3(fsim3),
  mVerify(verify)
{
}

// @brief デストラクタ
Op1::~Op1()
{
}

// @brief テストパタンが見つかった場合に呼ばれる関数
// @param[in] f 故障
// @param[in] val_list "入力ノードの番号 x 2 + 値" のリスト
void
Op1::set_detected(SaFault* f,
		  const vector<ymuint>& val_list)
{
  TestVector* tv = mTvMgr.new_vector();
  tv->init();
  for (vector<ymuint>::const_iterator p = val_list.begin();
       p != val_list.end(); ++ p) {
    ymuint tmp = *p;
    ymuint iid = tmp / 2;
    ymuint val = tmp % 2;
    if ( val == 1 ) {
      tv->set_val(iid, kVal1);
    }
    else {
      tv->set_val(iid, kVal0);
    }
  }
  if ( mVerify ) {
    bool detect = mFsim3.run(tv, f);
    assert_cond( detect , __FILE__, __LINE__);
  }

  mTvList.push_back(tv);

  mFaultMgr.set_status(f, kFsDetected);
}

// @brief 検出不能のときに呼ばれる関数
void
Op1::set_untestable(SaFault* f)
{
  mFaultMgr.set_status(f, kFsUntestable);
}

END_NAMESPACE_YM_SATPG
