
/// @file Lut443Match.cc
/// @brief Lut443Match の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2013 Yusuke Matsunaga
/// All rights reserved.


#include "Lut443Match.h"
#include "GbmNaive.h"
#include "GbmNaiveOneHot.h"


BEGIN_NAMESPACE_YM

//////////////////////////////////////////////////////////////////////
// クラス Lut443Match
//////////////////////////////////////////////////////////////////////

BEGIN_NONAMESPACE

void
make_lut443(RcfNetwork& network,
	    ymuint ni,
	    ymuint id[])
{
  vector<RcfNodeHandle> input_list(ni);
  for (ymuint i = 0; i < ni; ++ i) {
    input_list[i] = network.new_input();
  }

  // LUT1 を作る．
  vector<RcfNodeHandle> tmp_inputs1(4);
  for (ymuint i = 0; i < 4; ++ i) {
    tmp_inputs1[i] = input_list[id[i]];
  }
  RcfNodeHandle lut1 = network.new_lut(tmp_inputs1);

  // LUT2 を作る．
  for (ymuint i = 0; i < 4; ++ i) {
    tmp_inputs1[i] = input_list[id[i + 4]];
  }
  RcfNodeHandle lut2 = network.new_lut(tmp_inputs1);

  // LUT3 を作る．
  vector<RcfNodeHandle> tmp_inputs2(3);
  tmp_inputs2[0] = lut1;
  tmp_inputs2[1] = lut2;
  tmp_inputs2[2] = input_list[id[8]];
  RcfNodeHandle lut3 = network.new_lut(tmp_inputs2);

  network.set_output(lut3);
}

END_NONAMESPACE

// @brief コンストラクタ
Lut443Match::Lut443Match()
{
  // LUT-A0 を作る．
  ymuint id_a0[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
  make_lut443(mA0, 9, id_a0);

  // LUT-A1 を作る．
  ymuint id_a1[] = { 0, 1, 2, 3, 0, 4, 5, 6, 7 };
  make_lut443(mA1, 8, id_a1);

  // LUT-A2 を作る．
  ymuint id_a2[] = { 0, 1, 2, 3, 0, 1, 4, 5, 6 };
  make_lut443(mA2, 7, id_a2);

  // LUT-A3 を作る．
  ymuint id_a3[] = { 0, 1, 2, 3, 0, 1, 2, 4, 5 };
  make_lut443(mA3, 6, id_a3);

  // LUT-B0 を作る．
  ymuint id_b0[] = { 0, 1, 2, 3, 4, 5, 6, 7, 0 };
  make_lut443(mB0, 8, id_b0);

  // LUT-B1 を作る．
  ymuint id_b1[] = { 0, 1, 2, 3, 1, 4, 5, 6, 0 };
  make_lut443(mB1, 7, id_b1);

  // LUT-B2 を作る．
  ymuint id_b2[] = { 0, 1, 2, 3, 1, 2, 4, 5, 0 };
  make_lut443(mB2, 6, id_b2);

  // LUT-C0 を作る．
  ymuint id_c0[] = { 0, 1, 2, 3, 0, 4, 5, 6, 0 };
  make_lut443(mC0, 7, id_c0);

  // LUT-C1 を作る．
  ymuint id_c1[] = { 0, 1, 2, 3, 0, 1, 4, 5, 0 };
  make_lut443(mC1, 7, id_c1);

}

// @brief デストラクタ
Lut443Match::~Lut443Match()
{
}

// @brief 与えられた関数を LUT ネットワークで実現できるか調べる．
// @param[in] func 対象の関数
// @param[in] solver GBM ソルバ
bool
Lut443Match::match(const TvFunc& func,
		   GbmSolver& solver)
{
  cout << "Lut443Match::match(" << func << ")" << endl;

  ymuint ni = func.input_num();

  // まず関数の真のサポートを求める．
  vector<ymuint> sup_list;
  sup_list.reserve(ni);
  for (ymuint i = 0; i < ni; ++ i) {
    if ( func.check_sup(VarId(i)) ) {
      sup_list.push_back(i);
    }
  }

  TvFunc func1 = func;
  ymuint ni1 = sup_list.size();
  cout << " ni1 = " << ni1 << endl;
  if ( ni1 < ni ) {
    // 冗長な入力があった．
    ymuint nexp1 = (1U << ni1);
    vector<int> new_tv(nexp1);
    for (ymuint b = 0; b < nexp1; ++ b) {
      ymuint pos = 0;
      for (ymuint i = 0; i < ni1; ++ i) {
	if ( b & (1U << i) ) {
	  pos += (1U << sup_list[i]);
	}
      }
      new_tv[b] = func.value(pos);
    }
    func1 = TvFunc(ni1, new_tv);
  }

  if ( ni1 <= 5 ) {
    // 自明
    return true;
  }

  vector<bool> conf_bits;
  vector<ymuint> iorder;

  if ( ni1 == 6 ) {
    bool stat = solver.solve(mA3, func1, conf_bits, iorder);
    if ( stat ) {
      cout << "Type A-3 Match" << endl;
      cout << "  iorder = ";
      for (ymuint i = 0; i < ni1; ++ i) {
	cout << " " << iorder[i];
      }
      cout << endl;
      return true;
    }

    stat = solver.solve(mB2, func1, conf_bits, iorder);
    if ( stat ) {
      cout << "Type B-2 Match" << endl;
      cout << "  iorder = ";
      for (ymuint i = 0; i < ni1; ++ i) {
	cout << " " << iorder[i];
      }
      cout << endl;
      return true;
    }

    stat = solver.solve(mC1, func1, conf_bits, iorder);
    if ( stat ) {
      cout << "Type C-1 Match" << endl;
      cout << "  iorder = ";
      for (ymuint i = 0; i < ni1; ++ i) {
	cout << " " << iorder[i];
      }
      cout << endl;
      return true;
    }
  }

  if ( ni1 == 7 ) {
    bool stat = solver.solve(mA2, func1, conf_bits, iorder);
    if ( stat ) {
      cout << "Type A-2 Match" << endl;
      cout << "  iorder = ";
      for (ymuint i = 0; i < ni1; ++ i) {
	cout << " " << iorder[i];
      }
      cout << endl;
      return true;
    }

    stat = solver.solve(mB1, func1, conf_bits, iorder);
    if ( stat ) {
      cout << "Type B-1 Match" << endl;
      cout << "  iorder = ";
      for (ymuint i = 0; i < ni1; ++ i) {
	cout << " " << iorder[i];
      }
      cout << endl;
      return true;
    }

    stat = solver.solve(mC0, func1, conf_bits, iorder);
    if ( stat ) {
      cout << "Type C-0 Match" << endl;
      cout << "  iorder = ";
      for (ymuint i = 0; i < ni1; ++ i) {
	cout << " " << iorder[i];
      }
      cout << endl;
      return true;
    }
  }

  if ( ni1 == 8 ) {
    bool stat = solver.solve(mA1, func1, conf_bits, iorder);
    if ( stat ) {
      cout << "Type A-1 Match" << endl;
      cout << "  iorder = ";
      for (ymuint i = 0; i < ni1; ++ i) {
	cout << " " << iorder[i];
      }
      cout << endl;
      return true;
    }

    stat = solver.solve(mB0, func1, conf_bits, iorder);
    if ( stat ) {
      cout << "Type B-0 Match" << endl;
      cout << "  iorder = ";
      for (ymuint i = 0; i < ni1; ++ i) {
	cout << " " << iorder[i];
      }
      cout << endl;
      return true;
    }
  }

  if ( ni1 == 9 ) {
    bool stat = solver.solve(mA0, func1, conf_bits, iorder);
    if ( stat ) {
      cout << "Type A-0 Match" << endl;
      cout << "  iorder = ";
      for (ymuint i = 0; i < ni1; ++ i) {
	cout << " " << iorder[i];
      }
      cout << endl;
      return true;
    }
  }

  cout << "Not found" << endl;
  return false;
}

END_NAMESPACE_YM
