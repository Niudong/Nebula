/*******************************************************************************
 * Project:  Nebula
 * @file     CodecProto.cpp
 * @brief 
 * @author   Bwar
 * @date:    2016年8月11日
 * @note
 * Modify history:
 ******************************************************************************/

#include "util/logger/NetLogger.hpp"
#include "CodecProto.hpp"

namespace neb
{

CodecProto::CodecProto(std::shared_ptr<NetLogger> pLogger, E_CODEC_TYPE eCodecType, const std::string& strKey)
    : Codec(pLogger, eCodecType, strKey)
{
}

CodecProto::~CodecProto()
{
}

E_CODEC_STATUS CodecProto::Encode(const MsgHead& oMsgHead, const MsgBody& oMsgBody, CBuffer* pBuff)
{
    LOG4_TRACE("pBuff->ReadableBytes()=%u, oMsgHead.ByteSize() = %d", pBuff->ReadableBytes(), oMsgHead.ByteSize());
    int iHadWriteLen = 0;
    int iWriteLen = 0;
    int iNeedWriteLen = gc_uiMsgHeadSize;
    iWriteLen = pBuff->Write(oMsgHead.SerializeAsString().c_str(), gc_uiMsgHeadSize);
    if (iWriteLen != iNeedWriteLen)
    {
        LOG4_ERROR("buff write head iWriteLen != iNeedWriteLen!");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
        return(CODEC_STATUS_ERR);
    }
    iHadWriteLen += iWriteLen;
    if (oMsgHead.len() <= 0)    // 无包体（心跳包等），nebula在proto3的使用上以-1表示包体长度为0
    {
        return(CODEC_STATUS_OK);
    }
    iNeedWriteLen = oMsgBody.ByteSize();
    iWriteLen = pBuff->Write(oMsgBody.SerializeAsString().c_str(), oMsgBody.ByteSize());
    if (iWriteLen == iNeedWriteLen)
    {
        return(CODEC_STATUS_OK);
    }
    else
    {
        LOG4_ERROR("buff write body iWriteLen != iNeedWriteLen!");
        pBuff->SetWriteIndex(pBuff->GetWriteIndex() - iHadWriteLen);
        return(CODEC_STATUS_ERR);
    }
}

E_CODEC_STATUS CodecProto::Decode(CBuffer* pBuff, MsgHead& oMsgHead, MsgBody& oMsgBody)
{
    LOG4_TRACE("pBuff->ReadableBytes()=%d, pBuff->GetReadIndex()=%d",
                    pBuff->ReadableBytes(), pBuff->GetReadIndex());
    if (pBuff->ReadableBytes() >= gc_uiMsgHeadSize)
    {
        bool bResult = oMsgHead.ParseFromArray(pBuff->GetRawReadBuffer(), gc_uiMsgHeadSize);
        if (bResult)
        {
            LOG4_TRACE("pBuff->ReadableBytes()=%d, oMsgHead.len()=%d",
                            pBuff->ReadableBytes(), oMsgHead.len());
            if (oMsgHead.len() <= 0)      // 无包体（心跳包等），nebula在proto3的使用上以-1表示包体长度为0
            {
                pBuff->SkipBytes(gc_uiMsgHeadSize);
                return(CODEC_STATUS_OK);
            }
            if (pBuff->ReadableBytes() >= gc_uiMsgHeadSize + oMsgHead.len())
            {
                bResult = oMsgBody.ParseFromArray(
                                pBuff->GetRawReadBuffer() + gc_uiMsgHeadSize, oMsgHead.len());
                LOG4_TRACE("pBuff->ReadableBytes()=%d, oMsgBody.ByteSize()=%d", pBuff->ReadableBytes(), oMsgBody.ByteSize());
                if (bResult)
                {
                    pBuff->SkipBytes(gc_uiMsgHeadSize + oMsgHead.len());
                    return(CODEC_STATUS_OK);
                }
                else
                {
                    LOG4_ERROR("cmd[%u], seq[%lu] oMsgBody.ParseFromArray() error!", oMsgHead.cmd(), oMsgHead.seq());
                    return(CODEC_STATUS_ERR);
                }
            }
            else
            {
                return(CODEC_STATUS_PAUSE);
            }
        }
        else
        {
            LOG4_ERROR("oMsgHead.ParseFromArray() error!");
            return(CODEC_STATUS_ERR);
        }
    }
    else
    {
        return(CODEC_STATUS_PAUSE);
    }
}

} /* namespace neb */
