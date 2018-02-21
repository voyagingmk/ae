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
        printf("[kcp.send] err %d \n", ret);
        if (ret == -1)
        {
            printf("[kcp.send] len < 0 \n");
        }
        if (ret == -2)
        {
            printf("[kcp.send] seg == NULL or count > 255 \n");
        }
    }
    return ret;
}

int KCPObject::recv(char *buf, int len)
{
    int ret = ikcp_recv(m_kcp, buf, len);
    if (ret < 0)
    {
        printf("[kcp.recv] err %d \n", ret);
        if (ret == -1)
        {
            printf("[kcp.recv] rcv_queue is empty \n");
        }
        if (ret == -2)
        {
            printf("[kcp.recv] peeksize < 0 \n");
        }

        if (ret == -3)
        {
            printf("[kcp.recv] peeksize > len \n");
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
        printf("[kcp.input] err %d \n", ret);
        if (ret == -1)
        {
            printf("[kcp.input] wrong data \n");
        }
        if (ret == -2)
        {
            printf("[kcp.input] size < len \n");
        }

        if (ret == -3)
        {
            printf("[kcp.input] wrong cmd \n");
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
        printf("[kcp.setmtu] err %d \n", ret);
        if (ret == -1)
        {
            printf("[kcp.setmtu] mtu < 50 || mtu < (int)IKCP_OVERHEAD \n");
        }
        if (ret == -2)
        {
            printf("[kcp.setmtu] buffer == NULL \n");
        }
    }
    return ret;
}

int KCPObject::waitsnd()
{
    return ikcp_waitsnd(m_kcp);
}
};