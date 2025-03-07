// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "TCP_Options.h"
#include "DEV9/PacketReader/IP/IP_Packet.h"

namespace PacketReader::IP::TCP
{
	class TCP_Packet : public IP_Payload
	{
	public:
		u16 sourcePort;
		u16 destinationPort;
		u32 sequenceNumber;
		u32 acknowledgementNumber;

	private:
		u8 dataOffsetAndNS_Flag = 0;
		int headerLength; //Can have varying Header Len
		u8 flags = 0;

	public:
		u16 windowSize;

	private:
		u16 checksum;
		u16 urgentPointer = 0;

	public:
		std::vector<BaseOption*> options;

	private:
		const static IP_Type protocol = IP_Type::TCP;

		std::unique_ptr<Payload> payload;

	public:
		//Flags
		bool GetNS() const;
		void SetNS(bool value);

		bool GetCWR() const;
		void SetCWR(bool value);

		bool GetECE() const;
		void SetECE(bool value);

		bool GetURG() const;
		void SetURG(bool value);

		bool GetACK() const;
		void SetACK(bool value);

		bool GetPSH() const;
		void SetPSH(bool value);

		bool GetRST() const;
		void SetRST(bool value);

		bool GetSYN() const;
		void SetSYN(bool value);

		bool GetFIN() const;
		void SetFIN(bool value);

		//Takes ownership of payload
		TCP_Packet(Payload* data);
		TCP_Packet(const u8* buffer, int bufferSize);
		TCP_Packet(const TCP_Packet&);

		Payload* GetPayload() const;

		virtual int GetLength();
		virtual void WriteBytes(u8* buffer, int* offset);
		virtual TCP_Packet* Clone() const;

		virtual u8 GetProtocol() const;

		virtual bool VerifyChecksum(IP_Address srcIP, IP_Address dstIP);
		virtual void CalculateChecksum(IP_Address srcIP, IP_Address dstIP);

	private:
		void ReComputeHeaderLen();
	};
} // namespace PacketReader::IP::TCP
