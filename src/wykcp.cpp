#include "wykcp.h"

namespace wynet
{
KCPObject::KCPObject(int conv, void *userdata, OutputFunc outputFunc)
{
    m_kcp = ikcp_create(conv, userdata);
    m_kcp->output = outputFunc;
}

KCPObject::~KCPObject()
{
    ikcp_release(m_kcp);
    m_kcp = NULL;
}

const ikcpcb *KCPObject::kcp()
{
    return m_kcp;
}

int KCPObject::getSendWin()
{
    return m_kcp->snd_wnd;
}

int KCPObject::getRecvWin()
{
    return m_kcp->rcv_wnd;
}

void KCPObject::setSendWin(int wnd)
{
    if (wnd > 0)
    {
        m_kcp->snd_wnd = wnd;
    }
}

void KCPObject::setRecvWin(int wnd)
{
    if (wnd > 0)
    {
        m_kcp->rcv_wnd = wnd;
    }
}

void KCPObject::setNodelay(int nodelay, int interval, int resend, int nc)
{
    ikcp_nodelay(m_kcp, nodelay, interval, resend, nc);
}

int KCPObject::send(const char *buf, int len)
{
    int ret = ikcp_send(m_kcp, buf, len);
    if (ret < 0)
    {
        log_error("[kcp.send] err %d", ret);
        if (ret == -1)
        {
            log_error("[kcp.send] len < 0");
        }
        if (ret == -2)
        {
            log_error("[kcp.send] seg == NULL or count > 255");
        }
    }
    return ret;
}

int KCPObject::recv(char *buf, int len)
{
    int ret = ikcp_recv(m_kcp, buf, len);
    if (ret < 0)
    {
        log_error("[kcp.recv] err %d", ret);
        if (ret == -1)
        {
            log_error("[kcp.recv] rcv_queue is empty");
        }
        if (ret == -2)
        {
            log_error("[kcp.recv] peeksize < 0");
        }

        if (ret == -3)
        {
            log_error("[kcp.recv] peeksize > len");
        }
    }
    return ret;
}

int KCPObject::nextRecvSize()
{
    return ikcp_peeksize(m_kcp);
}

void KCPObject::update(IUINT32 current)
{
    ikcp_update(m_kcp, current);
}

IUINT32 KCPObject::check(IUINT32 current)
{
    return ikcp_check(m_kcp, current);
}

int KCPObject::input(const char *data, long size)
{
    int ret = ikcp_input(m_kcp, data, size);
    if (ret < 0)
    {
        log_error("[kcp.input] err %d", ret);
        if (ret == -1)
        {
            log_error("[kcp.input] wrong data");
        }
        if (ret == -2)
        {
            log_error("[kcp.input] size < len");
        }

        if (ret == -3)
        {
            log_error("[kcp.input] wrong cmd");
        }
    }
    return ret;
}

void KCPObject::flush()
{
    ikcp_flush(m_kcp);
}

int KCPObject::setmtu(int mtu)
{
    int ret = ikcp_setmtu(m_kcp, mtu);
    if (ret < 0)
    {
        log_error("[kcp.setmtu] err %d", ret);
        if (ret == -1)
        {
            log_error("[kcp.setmtu] mtu < 50 || mtu < (int)IKCP_OVERHEAD");
        }
        if (ret == -2)
        {
            log_error("[kcp.setmtu] buffer == NULL");
        }
    }
    return ret;
}

int KCPObject::waitsnd()
{
    return ikcp_waitsnd(m_kcp);
}
};