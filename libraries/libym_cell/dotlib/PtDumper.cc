
/// @file libym_cell/dotlib/PtDumper.cc
/// @brief PtDumper の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2011 Yusuke Matsunaga
/// All rights reserved.


#include "PtDumper.h"
#include "PtNode.h"
#include "PtLibrary.h"
#include "PtCell.h"
#include "PtBundle.h"
#include "PtBus.h"
#include "PtPin.h"
#include "PtTiming.h"
#include "PtTable.h"
#include "PtValue.h"


BEGIN_NAMESPACE_YM_CELL_DOTLIB

//////////////////////////////////////////////////////////////////////
// クラス PtDumper
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
PtDumper::PtDumper()
{
}

// @brief デストラクタ
PtDumper::~PtDumper()
{
}


BEGIN_NONAMESPACE

string
indent_str(ymuint indent)
{
  string ans;
  for (ymuint i = 0; i < indent; ++ i) {
    ans += ' ';
  }
  return ans;
}

void
dump_sub(ostream& s,
	 const PtNode* node,
	 ymuint indent)
{
#if 0
  ymuint nv = node->value_num();
  for (ymuint i = 0; i < nv; ++ i) {
    s << indent_str(indent)
      << "Value#" << i << ": " << node->value(i) << endl;
  }

  ymuint nc = node->child_num();
  for (ymuint i = 0; i < nc; ++ i) {
    s << indent_str(indent)
      << "Child#" << i << ":" << endl;
    dump_sub(s, node->child(i), indent + 2);
  }

  ymuint nk = node->child_attr_num();
  for (ymuint i = 0; i < nk; ++ i) {
    ShString attr = node->child_attr_name(i);
    s << indent_str(indent)
      << "Attr:" << attr << endl;
    ymuint nc = node->child_num(attr);
    for (ymuint j = 0; j < nc; ++ j) {
      s << indent_str(indent + 2)
	<< "Child#" << j << ":" << endl;
      dump_sub(s, node->child(attr, j), indent + 2);
    }
  }
#endif
}

END_NONAMESPACE


// @brief root 以下の内容を出力する．
// @param[in] s 出力先のストリーム
// @param[in] root パース木のノード
void
PtDumper::operator()(ostream& s,
		     const PtLibrary* root)
{
  dump_sub(s, root, 0);
}


END_NAMESPACE_YM_CELL_DOTLIB
