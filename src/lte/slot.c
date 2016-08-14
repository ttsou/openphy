/*
 * LTE Slot and Resource Block Parameters 
 *
 * Copyright (C) 2015 Ettus Research LLC
 * Author Tom Tsou <tom.tsou@ettus.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "slot.h"

static const int rb_pos_map_n6[6] = {
	LTE_N6_RB0,
	LTE_N6_RB1,
	LTE_N6_RB2,
	LTE_N6_RB3,
	LTE_N6_RB4,
	LTE_N6_RB5,
};

static const int rb_pos_map_n15[15] = {
	LTE_N15_RB0,  LTE_N15_RB1,  LTE_N15_RB2,  LTE_N15_RB3,
	LTE_N15_RB4,  LTE_N15_RB5,  LTE_N15_RB6,  LTE_N15_RB7,
	LTE_N15_RB8,  LTE_N15_RB9,  LTE_N15_RB10, LTE_N15_RB11,
	LTE_N15_RB12, LTE_N15_RB13, LTE_N15_RB14,
};

static const int rb_pos_map_n25[25] = {
	LTE_N25_RB0,  LTE_N25_RB1,  LTE_N25_RB2,  LTE_N25_RB3,
	LTE_N25_RB4,  LTE_N25_RB5,  LTE_N25_RB6,  LTE_N25_RB7,
	LTE_N25_RB8,  LTE_N25_RB9,  LTE_N25_RB10, LTE_N25_RB11,
	LTE_N25_RB12, LTE_N25_RB13, LTE_N25_RB14, LTE_N25_RB15,
	LTE_N25_RB16, LTE_N25_RB17, LTE_N25_RB18, LTE_N25_RB19,
	LTE_N25_RB20, LTE_N25_RB21, LTE_N25_RB22, LTE_N25_RB23,
	LTE_N25_RB24,
};

static const int rb_pos_map_n50[50] = {
	LTE_N50_RB0,  LTE_N50_RB1,  LTE_N50_RB2,  LTE_N50_RB3,
	LTE_N50_RB4,  LTE_N50_RB5,  LTE_N50_RB6,  LTE_N50_RB7,
	LTE_N50_RB8,  LTE_N50_RB9,  LTE_N50_RB10, LTE_N50_RB11,
	LTE_N50_RB12, LTE_N50_RB13, LTE_N50_RB14, LTE_N50_RB15,
	LTE_N50_RB16, LTE_N50_RB17, LTE_N50_RB18, LTE_N50_RB19,
	LTE_N50_RB20, LTE_N50_RB21, LTE_N50_RB22, LTE_N50_RB23,
	LTE_N50_RB24, LTE_N50_RB25, LTE_N50_RB26, LTE_N50_RB27,
	LTE_N50_RB28, LTE_N50_RB29, LTE_N50_RB30, LTE_N50_RB31,
	LTE_N50_RB32, LTE_N50_RB33, LTE_N50_RB34, LTE_N50_RB35,
	LTE_N50_RB36, LTE_N50_RB37, LTE_N50_RB38, LTE_N50_RB39,
	LTE_N50_RB40, LTE_N50_RB41, LTE_N50_RB42, LTE_N50_RB43,
	LTE_N50_RB44, LTE_N50_RB45, LTE_N50_RB46, LTE_N50_RB47,
	LTE_N50_RB48, LTE_N50_RB49,
};

static const int rb_pos_map_n75[75] = {
	LTE_N75_RB0,  LTE_N75_RB1,  LTE_N75_RB2,  LTE_N75_RB3,
	LTE_N75_RB4,  LTE_N75_RB5,  LTE_N75_RB6,  LTE_N75_RB7,
	LTE_N75_RB8,  LTE_N75_RB9,  LTE_N75_RB10, LTE_N75_RB11,
	LTE_N75_RB12, LTE_N75_RB13, LTE_N75_RB14, LTE_N75_RB15,
	LTE_N75_RB16, LTE_N75_RB17, LTE_N75_RB18, LTE_N75_RB19,
	LTE_N75_RB20, LTE_N75_RB21, LTE_N75_RB22, LTE_N75_RB23,
	LTE_N75_RB24, LTE_N75_RB25, LTE_N75_RB26, LTE_N75_RB27,
	LTE_N75_RB28, LTE_N75_RB29, LTE_N75_RB30, LTE_N75_RB31,
	LTE_N75_RB32, LTE_N75_RB33, LTE_N75_RB34, LTE_N75_RB35,
	LTE_N75_RB36, LTE_N75_RB37, LTE_N75_RB38, LTE_N75_RB39,
	LTE_N75_RB40, LTE_N75_RB41, LTE_N75_RB42, LTE_N75_RB43,
	LTE_N75_RB44, LTE_N75_RB45, LTE_N75_RB46, LTE_N75_RB47,
	LTE_N75_RB48, LTE_N75_RB49, LTE_N75_RB50, LTE_N75_RB51,
	LTE_N75_RB52, LTE_N75_RB53, LTE_N75_RB54, LTE_N75_RB55,
	LTE_N75_RB56, LTE_N75_RB57, LTE_N75_RB58, LTE_N75_RB59,
	LTE_N75_RB60, LTE_N75_RB61, LTE_N75_RB62, LTE_N75_RB63,
	LTE_N75_RB64, LTE_N75_RB65, LTE_N75_RB66, LTE_N75_RB67,
	LTE_N75_RB68, LTE_N75_RB69, LTE_N75_RB70, LTE_N75_RB71,
	LTE_N75_RB72, LTE_N75_RB73, LTE_N75_RB74,
};

static const int rb_pos_map_n100[100] = {
	LTE_N100_RB0,  LTE_N100_RB1,  LTE_N100_RB2,  LTE_N100_RB3,
	LTE_N100_RB4,  LTE_N100_RB5,  LTE_N100_RB6,  LTE_N100_RB7,
	LTE_N100_RB8,  LTE_N100_RB9,  LTE_N100_RB10, LTE_N100_RB11,
	LTE_N100_RB12, LTE_N100_RB13, LTE_N100_RB14, LTE_N100_RB15,
	LTE_N100_RB16, LTE_N100_RB17, LTE_N100_RB18, LTE_N100_RB19,
	LTE_N100_RB20, LTE_N100_RB21, LTE_N100_RB22, LTE_N100_RB23,
	LTE_N100_RB24, LTE_N100_RB25, LTE_N100_RB26, LTE_N100_RB27,
	LTE_N100_RB28, LTE_N100_RB29, LTE_N100_RB30, LTE_N100_RB31,
	LTE_N100_RB32, LTE_N100_RB33, LTE_N100_RB34, LTE_N100_RB35,
	LTE_N100_RB36, LTE_N100_RB37, LTE_N100_RB38, LTE_N100_RB39,
	LTE_N100_RB40, LTE_N100_RB41, LTE_N100_RB42, LTE_N100_RB43,
	LTE_N100_RB44, LTE_N100_RB45, LTE_N100_RB46, LTE_N100_RB47,
	LTE_N100_RB48, LTE_N100_RB49, LTE_N100_RB50, LTE_N100_RB51,
	LTE_N100_RB52, LTE_N100_RB53, LTE_N100_RB54, LTE_N100_RB55,
	LTE_N100_RB56, LTE_N100_RB57, LTE_N100_RB58, LTE_N100_RB59,
	LTE_N100_RB60, LTE_N100_RB61, LTE_N100_RB62, LTE_N100_RB63,
	LTE_N100_RB64, LTE_N100_RB65, LTE_N100_RB66, LTE_N100_RB67,
	LTE_N100_RB68, LTE_N100_RB69, LTE_N100_RB70, LTE_N100_RB71,
	LTE_N100_RB72, LTE_N100_RB73, LTE_N100_RB74, LTE_N100_RB75,
	LTE_N100_RB76, LTE_N100_RB77, LTE_N100_RB78, LTE_N100_RB79,
	LTE_N100_RB80, LTE_N100_RB81, LTE_N100_RB82, LTE_N100_RB83,
	LTE_N100_RB84, LTE_N100_RB85, LTE_N100_RB86, LTE_N100_RB87,
	LTE_N100_RB88, LTE_N100_RB89, LTE_N100_RB90, LTE_N100_RB91,
	LTE_N100_RB92, LTE_N100_RB93, LTE_N100_RB94, LTE_N100_RB95,
	LTE_N100_RB96, LTE_N100_RB97, LTE_N100_RB98, LTE_N100_RB99,
};

int lte_frame_len(int rbs)
{
	return 10 * lte_subframe_len(rbs);
}

int lte_subframe_len(int rbs)
{
	return 2 * lte_slot_len(rbs);
}

int lte_slot_len(int rbs)
{
	switch (rbs) {
	case 6:
		return LTE_N6_SLOT_LEN;
	case 15:
		return LTE_N15_SLOT_LEN;
	case 25:
		return LTE_N25_SLOT_LEN;
	case 50:
		return LTE_N50_SLOT_LEN;
	case 75:
		return LTE_N75_SLOT_LEN;
	case 100:
		return LTE_N100_SLOT_LEN;
	}

	return -1;
}

int lte_sym_len(int rbs)
{
	switch (rbs) {
	case 6:
		return LTE_N6_SYM_LEN;
	case 15:
		return LTE_N15_SYM_LEN;
	case 25:
		return LTE_N25_SYM_LEN;
	case 50:
		return LTE_N50_SYM_LEN;
	case 75:
		return LTE_N75_SYM_LEN;
	case 100:
		return LTE_N100_SYM_LEN;
	}

	return -1;
}

int lte_cp_len(int rbs)
{
	switch (rbs) {
	case 6:
		return LTE_N6_CP_LEN;
	case 15:
		return LTE_N15_CP_LEN;
	case 25:
		return LTE_N25_CP_LEN;
	case 50:
		return LTE_N50_CP_LEN;
	case 75:
		return LTE_N75_CP_LEN;
	case 100:
		return LTE_N100_CP_LEN;
	}

	return -1;
}

int lte_rb_pos(int rbs, int rb)
{
	if (rb >= rbs)
		return -1;

	switch (rbs) {
	case 6:
		return rb_pos_map_n6[rb];
	case 15:
		return rb_pos_map_n15[rb];
	case 25:
		return rb_pos_map_n25[rb];
	case 50:
		return rb_pos_map_n50[rb];
	case 75:
		return rb_pos_map_n75[rb];
	case 100:
		return rb_pos_map_n100[rb];
	}

	return -1;
}

int lte_rb_pos_mid(int rbs)
{
	switch (rbs) {
	case 6:
		return LTE_N6_RB3;
	case 15:
		return LTE_N15_RB7_1;
	case 25:
		return LTE_N25_RB12_1;
	case 50:
		return LTE_N50_RB25;
	case 75:
		return LTE_N75_RB37_1;
	case 100:
		return LTE_N100_RB50;
	}

	return -1;
}

#define SYM_POS_MAP(X) \
	int sym_pos_map_n##X[7] = { \
		LTE_N##X##_SYM0, \
		LTE_N##X##_SYM1, \
		LTE_N##X##_SYM2, \
		LTE_N##X##_SYM3, \
		LTE_N##X##_SYM4, \
		LTE_N##X##_SYM5, \
		LTE_N##X##_SYM6, \
	};

SYM_POS_MAP(6)
SYM_POS_MAP(15)
SYM_POS_MAP(25)
SYM_POS_MAP(50)
SYM_POS_MAP(75)
SYM_POS_MAP(100)

int lte_sym_pos(int rbs, int l)
{
	if ((l < 0) || (l > 6))
		return -1;

	switch (rbs) {
	case 6:
		return sym_pos_map_n6[l];
	case 15:
		return sym_pos_map_n15[l];
	case 25:
		return sym_pos_map_n25[l];
	case 50:
		return sym_pos_map_n50[l];
	case 75:
		return sym_pos_map_n75[l];
	case 100:
		return sym_pos_map_n100[l];
	}

	return -1;
}
