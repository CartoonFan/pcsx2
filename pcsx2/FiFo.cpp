// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "Common.h"

#include "Gif.h"
#include "Gif_Unit.h"
#include "MTGS.h"
#include "Vif.h"
#include "Vif_Dma.h"

//////////////////////////////////////////////////////////////////////////
/////////////////////////// Quick & dirty FIFO :D ////////////////////////
//////////////////////////////////////////////////////////////////////////

// Notes on FIFO implementation
//
// The FIFO consists of four separate pages of HW register memory, each mapped to a
// PS2 device.  They are listed as follows:
//
// 0x4000 - 0x5000 : VIF0  (all registers map to 0x4000)
// 0x5000 - 0x6000 : VIF1  (all registers map to 0x5000)
// 0x6000 - 0x7000 : GS    (all registers map to 0x6000)
// 0x7000 - 0x8000 : IPU   (registers map to 0x7000 and 0x7010, respectively)

void ReadFIFO_VIF1(mem128_t* out)
{
	if (vif1Regs.stat.test(VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
		DevCon.Warning("Reading from vif1 fifo when stalled");

	ZeroQWC(out); // Clear first in case no data gets written...
	pxAssertRel(vif1Regs.stat.FQC != 0, "FQC = 0 on VIF FIFO READ!");
	if (vif1Regs.stat.FDR)
	{
		if (vif1Regs.stat.FQC > vif1.GSLastDownloadSize)
		{
			DevCon.Warning("Warning! GS Download size < FIFO count!");
		}
		if (vif1Regs.stat.FQC > 0)
		{
			MTGS::InitAndReadFIFO(reinterpret_cast<u8*>(out), 1);
			vif1.GSLastDownloadSize--;
			GUNIT_LOG("ReadFIFO_VIF1");
			if (vif1.GSLastDownloadSize <= 16)
				gifRegs.stat.OPH = false;
			vif1Regs.stat.FQC = std::min((u32)16, vif1.GSLastDownloadSize);
		}
	}

	VIF_LOG("ReadFIFO/VIF1 -> 0x%08X.%08X.%08X.%08X", out->_u32[0], out->_u32[1], out->_u32[2], out->_u32[3]);
}

//////////////////////////////////////////////////////////////////////////
// WriteFIFO Pages
//
void WriteFIFO_VIF0(const mem128_t* value)
{
	VIF_LOG("WriteFIFO/VIF0 <- 0x%08X.%08X.%08X.%08X", value->_u32[0], value->_u32[1], value->_u32[2], value->_u32[3]);

	vif0ch.qwc += 1;
	if (vif0.irqoffset.value != 0 && vif0.vifstalled.enabled)
		DevCon.Warning("Offset on VIF0 FIFO start!");
	[[maybe_unused]] bool ret = VIF0transfer((u32*)value, 4);

	if (vif0.cmd)
	{
		if (vif0.done && vif0ch.qwc == 0)
			vif0Regs.stat.VPS = VPS_WAITING;
	}
	else
	{
		vif0Regs.stat.VPS = VPS_IDLE;
	}

	pxAssertMsg(ret, "vif stall code not implemented");
}

void WriteFIFO_VIF1(const mem128_t* value)
{
	VIF_LOG("WriteFIFO/VIF1 <- 0x%08X.%08X.%08X.%08X", value->_u32[0], value->_u32[1], value->_u32[2], value->_u32[3]);

	if (vif1Regs.stat.FDR)
	{
		DevCon.Warning("writing to fifo when fdr is set!");
	}
	if (vif1Regs.stat.test(VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
	{
		DevCon.Warning("writing to vif1 fifo when stalled");
	}
	if (vif1.irqoffset.value != 0 && vif1.vifstalled.enabled)
	{
		DevCon.Warning("Offset on VIF1 FIFO start!");
	}

	[[maybe_unused]] bool ret = VIF1transfer((u32*)value, 4);

	if (vif1.cmd)
	{
		if (vif1.done && !vif1ch.qwc)
			vif1Regs.stat.VPS = VPS_WAITING;
	}
	else
		vif1Regs.stat.VPS = VPS_IDLE;

	if (gifRegs.stat.APATH == 2 && gifUnit.gifPath[1].isDone())
	{
		gifRegs.stat.APATH = 0;
		gifRegs.stat.OPH = 0;
		vif1Regs.stat.VGW = false; //Let vif continue if it's stuck on a flush

		if (gifUnit.checkPaths(1, 0, 1))
			gifUnit.Execute(false, true);
	}

	pxAssertMsg(ret, "vif stall code not implemented");
}

void WriteFIFO_GIF(const mem128_t* value)
{
	GUNIT_LOG("WriteFIFO_GIF()");
	if ((!gifUnit.CanDoPath3() || gif_fifo.fifoSize > 0))
	{
		//DevCon.Warning("GIF FIFO HW Write");
		gif_fifo.write_fifo((u32*)value, 1);
		gif_fifo.read_fifo();
	}
	else
	{
		gifUnit.TransferGSPacketData(GIF_TRANS_FIFO, (u8*)value, 16);
	}

	if (gifUnit.gifPath[GIF_PATH_3].state == GIF_PATH_WAIT)
		gifUnit.gifPath[GIF_PATH_3].state = GIF_PATH_IDLE;

	if (gifRegs.stat.APATH == 3)
	{
		gifRegs.stat.APATH = 0;
		gifRegs.stat.OPH = 0;

		if (gifUnit.gifPath[GIF_PATH_3].state == GIF_PATH_IDLE || gifUnit.gifPath[GIF_PATH_3].state == GIF_PATH_WAIT)
		{
			if (gifUnit.checkPaths(1, 1, 0))
				gifUnit.Execute(false, true);
		}
	}
}
