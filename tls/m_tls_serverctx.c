/* The MIT License (MIT)
 * 
 * Copyright (c) 2017 Monetra Technologies, LLC.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "m_config.h"
#include <mstdlib/mstdlib_io.h>
#include <mstdlib/io/m_io_layer.h>
#include <mstdlib/mstdlib_thread.h>
#include <mstdlib/mstdlib_tls.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h> /* For X509_check_host() */
#include <openssl/pem.h>
#include <openssl/err.h>
#if OPENSSL_VERSION_NUMBER >= 0x3000000fL
#  include <openssl/core_names.h>
#  include <openssl/param_build.h>
#endif
#include "base/m_defs_int.h"
#include "m_tls_serverctx_int.h"
#include "m_tls_hostvalidate.h"

static int M_tls_serverctx_sni_cb(SSL *ssl, int *ad, void *arg)
{
	M_tls_serverctx_t *ctx      = arg;
	M_tls_serverctx_t *child    = NULL;
	const char        *hostname;
	int                retval   = SSL_TLSEXT_ERR_OK;

	(void)ad;

	if (ssl == NULL || ctx == NULL)
		return SSL_TLSEXT_ERR_NOACK;

	hostname = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
	if (M_str_isempty(hostname))
		return SSL_TLSEXT_ERR_NOACK;

	/* Shouldn't happen */
	if (ctx->parent)
		ctx = ctx->parent;

	M_thread_mutex_lock(ctx->lock);

	child = M_tls_serverctx_SNI_lookup(ctx, hostname);

	/* Only swap if found match and match is not the parent */
	if (child != NULL && ctx != child) {
		M_thread_mutex_lock(child->lock);
		if (!SSL_set_SSL_CTX(ssl, child->ctx)) {
			retval = SSL_TLSEXT_ERR_NOACK;
		}
		M_thread_mutex_unlock(child->lock);
	}
	M_thread_mutex_unlock(ctx->lock);

	return retval;
}


static EVP_PKEY *M_tls_dhparams_to_pkey(BIGNUM *p, BIGNUM *g)
{
#if OPENSSL_VERSION_NUMBER < 0x3000000fL
	EVP_PKEY *dh     = EVP_PKEY_new();
	DH       *dhtemp = DH_new();

	if (DH_set0_pqg(dhtemp, BN_dup(p), NULL /* q */, BN_dup(g)) != 1)
		goto end;

	if (EVP_PKEY_assign_DH(dh, dhtemp) != 1)
		goto end;

	dhtemp = NULL; /* assigned to pkey */

end:
	DH_free(dhtemp);
	return dh;

#else
	EVP_PKEY_CTX *ctx       = EVP_PKEY_CTX_new_id(EVP_PKEY_DH, NULL);
	EVP_PKEY     *dh        = NULL;
#  if 0
	/* This doesn't work for some reason, have to go through param builder */
	OSSL_PARAM    params[3] = {
		OSSL_PARAM_BN(OSSL_PKEY_PARAM_FFC_P, p, BN_num_bytes(p)),
		OSSL_PARAM_BN(OSSL_PKEY_PARAM_FFC_G, g, BN_num_bytes(g)),
		OSSL_PARAM_END
	};
#  endif
	OSSL_PARAM_BLD *param_bld = NULL;
	OSSL_PARAM     *params    = NULL;

	if (ctx == NULL)
		goto end;

	if (EVP_PKEY_fromdata_init(ctx) <= 0)
		goto end;

	param_bld = OSSL_PARAM_BLD_new();
	if (!OSSL_PARAM_BLD_push_BN(param_bld, OSSL_PKEY_PARAM_FFC_P, p))
		goto end;

	if (!OSSL_PARAM_BLD_push_BN(param_bld, OSSL_PKEY_PARAM_FFC_G, g))
		goto end;

	params = OSSL_PARAM_BLD_to_param(param_bld);
	if (params == NULL)
		goto end;

	if (EVP_PKEY_fromdata(ctx, &dh, EVP_PKEY_KEYPAIR, params) <= 0)
		goto end;

end:
	OSSL_PARAM_BLD_free(param_bld);
	OSSL_PARAM_free(params);
	EVP_PKEY_CTX_free(ctx);
	return dh;
#endif
}

/* Generated by "openssl dhparam -C 2236" */
static EVP_PKEY *M_tls_dh2236(void)
{
	static unsigned char dh2236_p[] = {
		0x09,0x5B,0xED,0x9D,0x7B,0xA5,0xB8,0xF7,0xAE,0x67,0x01,0xCD,
		0xE9,0x48,0x9A,0xAD,0x97,0xE6,0x38,0x6C,0x66,0x33,0x93,0xBD,
		0x3E,0x2C,0x59,0x1E,0xB4,0x34,0x3C,0xDB,0xE3,0x3E,0xC2,0x4F,
		0xFB,0xC4,0x5F,0x91,0x07,0x1A,0xF2,0xDB,0xDB,0xFC,0xA4,0x5D,
		0x75,0xBB,0x28,0x72,0x98,0xFE,0x65,0x75,0x9B,0x44,0xB3,0xE1,
		0x20,0xB2,0x40,0xA1,0xE6,0x20,0x4E,0x0F,0x8B,0xD2,0x8A,0xAB,
		0x1A,0xB7,0x23,0x57,0xF1,0x44,0xBC,0x82,0xB9,0x5A,0x3E,0x56,
		0xEE,0x15,0x90,0x2E,0x23,0xF3,0x30,0x12,0x42,0xE9,0x24,0x34,
		0x48,0xF0,0x37,0x97,0x96,0x5C,0xBA,0x63,0x85,0x35,0x66,0xCB,
		0xF9,0x85,0x8E,0xDD,0xEA,0xE2,0xD0,0x5B,0xA3,0xDB,0x7E,0x6F,
		0x44,0x2F,0x54,0xBC,0x99,0x8F,0x85,0xD2,0x0A,0x50,0xE3,0x74,
		0x40,0xEF,0xDF,0x2E,0x85,0xC8,0x1B,0x68,0xDE,0x38,0x12,0x9A,
		0xE7,0x63,0x32,0x73,0x1F,0x7D,0xB1,0xCB,0xCF,0x9A,0xDA,0xCE,
		0xD2,0x02,0xC7,0xC1,0xE3,0xE8,0xFA,0xFD,0x2E,0xAF,0xE5,0x7E,
		0xD3,0x7B,0xD8,0xFC,0x0D,0x2E,0x40,0xC4,0x4F,0xB3,0xD9,0xFB,
		0xF4,0x79,0x3E,0xA9,0xF5,0xEC,0xC3,0xE0,0x88,0xC6,0x90,0xC0,
		0x53,0x40,0xBF,0x7C,0xA5,0xEF,0x29,0xFE,0xBD,0x2E,0x27,0xC7,
		0x5A,0xFB,0xD6,0x21,0x5E,0xEB,0x50,0xF6,0x98,0x7E,0x1E,0x19,
		0x1D,0x1D,0xF3,0xE5,0x9E,0x5A,0x1C,0x43,0x92,0x0C,0x55,0xF0,
		0x5B,0xAA,0x96,0x7A,0x4C,0x1E,0xF0,0xF6,0xDC,0x5C,0xF2,0xCE,
		0x95,0xC1,0x4B,0x92,0x34,0xBC,0x99,0xAB,0x33,0xFF,0x80,0x78,
		0xA8,0x47,0x24,0xEF,0xE8,0x7B,0x4F,0x50,0xCE,0x42,0x9D,0x47,
		0x2D,0x36,0x99,0xC2,0x1D,0x9B,0x36,0x34,0xA7,0xFC,0xFC,0x50,
		0xCD,0x41,0xC8,0x8B,
	};
	static unsigned char dh2236_g[] = {
		0x02,
	};

	BIGNUM       *p         = NULL;
	BIGNUM       *g         = NULL;
	EVP_PKEY     *dh        = NULL;

	p = BN_bin2bn(dh2236_p, sizeof(dh2236_p), NULL);
	if (p == NULL)
		goto end;

	g = BN_bin2bn(dh2236_g, sizeof(dh2236_g), NULL);
	if (g == NULL)
		goto end;

	dh = M_tls_dhparams_to_pkey(p, g);
	if (dh == NULL)
		goto end;

end:
	BN_free(p);
	BN_free(g);

	return dh;
}

static void M_tls_serverctx_set_session_support(SSL_CTX *sslctx, M_bool enable)
{
	if (enable) {
		SSL_CTX_set_session_cache_mode(sslctx, SSL_SESS_CACHE_SERVER);
	} else {
		SSL_CTX_set_session_cache_mode(sslctx, SSL_SESS_CACHE_OFF);
	}
}


M_tls_serverctx_t *M_tls_serverctx_create(const unsigned char *key, size_t key_len, const unsigned char *crt, size_t crt_len, const unsigned char *intermediate, size_t intermediate_len)
{
	M_tls_serverctx_t *ctx;
	SSL_CTX           *sslctx;
	X509              *x509     = NULL;

	M_tls_init(M_TLS_INIT_NORMAL);

	sslctx       = M_tls_ctx_init(M_TRUE);
	if (sslctx == NULL)
		return NULL;

	if (!M_tls_ctx_set_cert(sslctx, key, key_len, crt, crt_len, intermediate, intermediate_len, &x509)) {
		SSL_CTX_free(sslctx);
		return NULL;
	}

	ctx                         = M_malloc_zero(sizeof(*ctx));
	ctx->ctx                    = sslctx;
#if OPENSSL_VERSION_NUMBER < 0x1000200fL
	ctx->x509                   = x509;
#endif
	ctx->lock                   = M_thread_mutex_create(M_THREAD_MUTEXATTR_RECURSIVE);
	ctx->sessions_enabled       = M_TRUE;
	ctx->ref_cnt                = 1;
	ctx->negotiation_timeout_ms = 10000;

	/* DH default params */
	/* Use static DH parameters.  This logic comes from:
	 * http://en.wikibooks.org/wiki/OpenSSL/Diffie-Hellman_parameters
	 * And OpenSSL's docs say "Application authors may compile in DH parameters."
	 * We know the NSA can factor 1024bit static keys, but no evidence shows
	 * 2048-bit or greater can be.
	 */

	/* NOTE: due to the way openssl handles DH parameters, we have to delay actually
	 *       setting this until the SSL object is created otherwise we can't reliably
	 *       change things. */
	ctx->dh = M_tls_dh2236();

	/* SNI support */
	SSL_CTX_set_tlsext_servername_callback(sslctx, M_tls_serverctx_sni_cb);
	SSL_CTX_set_tlsext_servername_arg(sslctx, ctx);

	M_tls_serverctx_set_session_support(sslctx, ctx->sessions_enabled);

	return ctx;
}


M_tls_serverctx_t *M_tls_serverctx_create_from_files(const char *keypath, const char *crtpath, const char *intermediatepath)
{
	M_tls_serverctx_t *ctx;
	unsigned char     *crt       = NULL;
	unsigned char     *key       = NULL;
	unsigned char     *inter     = NULL;
	size_t             crt_len   = 0;
	size_t             key_len   = 0;
	size_t             inter_len = 0;

	if (M_str_isempty(keypath) || M_str_isempty(crtpath))
		return NULL;

	if (M_fs_file_read_bytes(crtpath, 0, &crt, &crt_len) != M_FS_ERROR_SUCCESS)
		return NULL;

	if (M_fs_file_read_bytes(keypath, 0, &key, &key_len) != M_FS_ERROR_SUCCESS) {
		M_free(crt);
		return NULL;
	}

	if (intermediatepath != NULL) {
		if (M_fs_file_read_bytes(intermediatepath, 0, &inter, &inter_len) != M_FS_ERROR_SUCCESS) {
			M_free(key);
			M_free(crt);
			return NULL;
		}
	}

	ctx = M_tls_serverctx_create(key, key_len, crt, crt_len, inter, inter_len);
	M_free(crt);
	M_free(key);
	M_free(inter);

	return ctx;
}


static void M_tls_serverctx_child_destroy(void *arg)
{
	M_tls_serverctx_t *ctx = arg;
	ctx->parent = NULL;
	M_tls_serverctx_destroy(ctx);
}


M_bool M_tls_serverctx_SNI_ctx_add(M_tls_serverctx_t *ctx, M_tls_serverctx_t *child)
{
	if (ctx == NULL || child == NULL)
		return M_FALSE;

	/* Can only be bound to one parent */
	if (child->parent)
		return M_FALSE;

	/* Can not bind children to a child */
	if (ctx->parent)
		return M_FALSE;

	M_thread_mutex_lock(ctx->lock);
	M_thread_mutex_lock(child->lock);

	child->parent = ctx;

	if (ctx->children == NULL) {
		const struct M_list_callbacks list_cbs = {
			NULL,
			NULL,
			NULL,
			M_tls_serverctx_child_destroy
		};
		ctx->children = M_list_create(&list_cbs, M_LIST_NONE);
	}

	M_list_insert(ctx->children, child);

	M_thread_mutex_unlock(child->lock);
	M_thread_mutex_unlock(ctx->lock);

	return M_TRUE;
}


size_t M_tls_serverctx_SNI_count(M_tls_serverctx_t *ctx)
{
	size_t count;

	if (ctx == NULL)
		return 0;

	M_thread_mutex_lock(ctx->lock);
	count = M_list_len(ctx->children);
	M_thread_mutex_unlock(ctx->lock);

	return count;
}


M_tls_serverctx_t *M_tls_serverctx_SNI_lookup(M_tls_serverctx_t *ctx, const char *hostname)
{
	M_tls_serverctx_t *child = NULL;
	size_t             i;
	X509              *x509;

	if (ctx == NULL)
		return NULL;

	M_thread_mutex_lock(ctx->lock);

	/* Check self */
#if OPENSSL_VERSION_NUMBER >= 0x1000200fL
	x509 = SSL_CTX_get0_certificate(ctx->ctx);
#else
	x509 = ctx->x509;
#endif
	if (M_tls_verify_host(x509, hostname, M_TLS_VERIFY_HOST_FLAG_NORMAL)) {
		M_thread_mutex_unlock(ctx->lock);
		return ctx;
	}

	for (i=0; i<M_list_len(ctx->children); i++) {
		const void *ptr = M_list_at(ctx->children, i);

		child = M_CAST_OFF_CONST(M_tls_serverctx_t *, ptr);

		M_thread_mutex_lock(child->lock);
#if OPENSSL_VERSION_NUMBER >= 0x1000200fL
		x509 = SSL_CTX_get0_certificate(child->ctx);
#else
		x509 = child->x509;
#endif
		if (x509 != NULL && M_tls_verify_host(x509, hostname, M_TLS_VERIFY_HOST_FLAG_NORMAL)) {
			/* Found a match, unlock child and break out of the loop */
			M_thread_mutex_unlock(child->lock);
			break;
		}
		M_thread_mutex_unlock(child->lock);
		child = NULL;
	}
	M_thread_mutex_unlock(ctx->lock);

	return child;
}


M_tls_serverctx_t *M_tls_serverctx_SNI_at(M_tls_serverctx_t *ctx, size_t idx)
{
	const void *ptr;

	if (ctx == NULL)
		return NULL;

	M_thread_mutex_lock(ctx->lock);
	ptr = M_list_at(ctx->children, idx);
	M_thread_mutex_unlock(ctx->lock);

	return M_CAST_OFF_CONST(M_tls_serverctx_t *, ptr);
}


char *M_tls_serverctx_get_cert(M_tls_serverctx_t *ctx)
{
	X509    *x509     = NULL;
	BIO     *bio      = NULL;
	char    *buf      = NULL;
	size_t   buf_size = 0;

	if (ctx == NULL)
		return NULL;

	M_thread_mutex_lock(ctx->lock);

#if OPENSSL_VERSION_NUMBER >= 0x1000200fL
	x509 = SSL_CTX_get0_certificate(ctx->ctx);
#else
	x509 = ctx->x509;
#endif

	/* Write as PEM encoded cert */
	bio = BIO_new(BIO_s_mem());
	if (bio == NULL) {
		goto done;
	}

	if (!PEM_write_bio_X509(bio, x509)) {
		goto done;
	}

	buf_size = (size_t)BIO_ctrl_pending(bio);
	buf      = M_malloc_zero(buf_size+1);
	if (buf == NULL) {
		goto done;
	}

	if ((size_t)BIO_read(bio, buf, (int)buf_size) != buf_size) {
		M_free(buf);
		buf = NULL;
		goto done;
	}

done:

	if (bio != NULL)
		BIO_free(bio);

	M_thread_mutex_unlock(ctx->lock);
	return buf;
}


M_bool M_tls_serverctx_upref(M_tls_serverctx_t *ctx)
{
	if (ctx == NULL)
		return M_FALSE;
	M_thread_mutex_lock(ctx->lock);
	ctx->ref_cnt++;
	M_thread_mutex_unlock(ctx->lock);
	return M_TRUE;
}


static void M_tls_serverctx_destroy_real(M_tls_serverctx_t *ctx)
{
	SSL_CTX_free(ctx->ctx);
	if (ctx->dh)
		EVP_PKEY_free(ctx->dh);
	M_list_destroy(ctx->children, M_TRUE);
	M_free(ctx->alpn_apps);

	/* Locked when entered */
	M_thread_mutex_unlock(ctx->lock);
	M_thread_mutex_destroy(ctx->lock);

	M_free(ctx);
}


void M_tls_serverctx_destroy(M_tls_serverctx_t *ctx)
{
	M_thread_mutex_lock(ctx->lock);

	if (ctx->ref_cnt == 1 && ctx->parent) {
		M_tls_serverctx_t *parent = ctx->parent;

		M_thread_mutex_lock(parent->lock);
		M_thread_mutex_unlock(ctx->lock);

		/* Detach from parent, parent will clean up child */
		M_list_remove_val(parent->children, ctx, M_LIST_MATCH_PTR);

		M_thread_mutex_unlock(parent->lock);
		return;
	}

	if (ctx->ref_cnt > 0) {
		ctx->ref_cnt--;
	}

	if (ctx->ref_cnt != 0) {
		M_thread_mutex_unlock(ctx->lock);
		return;
	}

	M_tls_serverctx_destroy_real(ctx);
}


M_bool M_tls_serverctx_set_protocols(M_tls_serverctx_t *ctx, int protocols)
{
	M_bool retval = M_TRUE;

	if (ctx == NULL)
		return M_FALSE;

	M_thread_mutex_lock(ctx->lock);
	retval = M_tls_ctx_set_protocols(ctx->ctx, protocols);
	M_thread_mutex_unlock(ctx->lock);

	return retval;
}


M_bool M_tls_serverctx_set_ciphers(M_tls_serverctx_t *ctx, const char *ciphers)
{
	M_bool retval = M_TRUE;

	if (ctx == NULL)
		return M_FALSE;

	M_thread_mutex_lock(ctx->lock);
	retval = M_tls_ctx_set_ciphers(ctx->ctx, ciphers);
	M_thread_mutex_unlock(ctx->lock);

	return retval;
}

M_bool M_tls_serverctx_set_server_preference(M_tls_serverctx_t *ctx, M_bool tf)
{
	if (ctx == NULL || ctx->ctx == NULL)
		return M_FALSE;

	if (tf) {
		SSL_CTX_set_options(ctx->ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
	} else {
		SSL_CTX_clear_options(ctx->ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
	}
	return M_TRUE;
}

/* Client certificate validation */
M_bool M_tls_serverctx_set_trust_ca(M_tls_serverctx_t *ctx, const unsigned char *ca, size_t len)
{
	M_bool retval = M_TRUE;

	if (ctx == NULL)
		return M_FALSE;

	M_thread_mutex_lock(ctx->lock);
	SSL_CTX_set_verify(ctx->ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT|SSL_VERIFY_CLIENT_ONCE, NULL);
	retval = M_tls_ctx_set_trust_ca(ctx->ctx, NULL, ca, len);
	M_thread_mutex_unlock(ctx->lock);

	return retval;
}


M_bool M_tls_serverctx_set_trust_ca_file(M_tls_serverctx_t *ctx, const char *path)
{
	M_bool retval = M_TRUE;

	if (ctx == NULL)
		return M_FALSE;

	M_thread_mutex_lock(ctx->lock);
	SSL_CTX_set_verify(ctx->ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT|SSL_VERIFY_CLIENT_ONCE, NULL);
	retval = M_tls_ctx_set_trust_ca_file(ctx->ctx, NULL, path);
	M_thread_mutex_unlock(ctx->lock);

	return retval;
}


M_bool M_tls_serverctx_set_trust_ca_dir(M_tls_serverctx_t *ctx, const char *path)
{
	M_bool retval = M_TRUE;

	if (ctx == NULL)
		return M_FALSE;

	M_thread_mutex_lock(ctx->lock);
	SSL_CTX_set_verify(ctx->ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT|SSL_VERIFY_CLIENT_ONCE, NULL);
	retval = M_tls_ctx_set_trust_ca_dir(ctx->ctx, NULL, path, "*.pem");
	M_thread_mutex_unlock(ctx->lock);

	return retval;
}


M_bool M_tls_serverctx_set_trust_cert(M_tls_serverctx_t *ctx, const unsigned char *crt, size_t len)
{
	M_bool retval = M_TRUE;

	if (ctx == NULL)
		return M_FALSE;

	M_thread_mutex_lock(ctx->lock);
	SSL_CTX_set_verify(ctx->ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT|SSL_VERIFY_CLIENT_ONCE, NULL);
	retval = M_tls_ctx_set_trust_cert(ctx->ctx, NULL, crt, len);
	M_thread_mutex_unlock(ctx->lock);

	return retval;
}


M_bool M_tls_serverctx_set_trust_cert_file(M_tls_serverctx_t *ctx, const char *path)
{
	M_bool retval = M_TRUE;

	if (ctx == NULL)
		return M_FALSE;

	M_thread_mutex_lock(ctx->lock);
	SSL_CTX_set_verify(ctx->ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT|SSL_VERIFY_CLIENT_ONCE, NULL);
	retval = M_tls_ctx_set_trust_cert_file(ctx->ctx, NULL, path);
	M_thread_mutex_unlock(ctx->lock);

	return retval;
}


static M_bool M_tls_serverctx_add_trust_crl_int(M_tls_serverctx_t *ctx, X509_CRL *crl)
{
	X509_STORE *store  = SSL_CTX_get_cert_store(ctx->ctx);
	X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK|X509_V_FLAG_CRL_CHECK_ALL);
	X509_STORE_add_crl(store, crl);

	return M_TRUE;
}


M_bool M_tls_serverctx_add_trust_crl(M_tls_serverctx_t *ctx, const unsigned char *crl, size_t len)
{
	BIO                 *bio;
	STACK_OF(X509_INFO) *sk;
	size_t               i;
	M_bool               rv    = M_FALSE;
	size_t               count = 0;

	if (ctx == NULL)
		return M_FALSE;

	/* Load CRL into BIO */
	bio = BIO_new_mem_buf(M_CAST_OFF_CONST(void *, crl), (int)len);

	/* Parse the CRL into a stack of X509 INFO structures */
	sk  = PEM_X509_INFO_read_bio(bio, NULL, NULL, NULL);
	if (sk == NULL)
		goto end;

	M_thread_mutex_lock(ctx->lock);
	for(i = 0; i < (size_t)sk_X509_INFO_num(sk); i++) {
		X509_INFO *x509info = sk_X509_INFO_value(sk, (int)i);
		if (x509info->crl != NULL) {
			if (M_tls_serverctx_add_trust_crl_int(ctx, x509info->crl)) {
				count++;
			}
		}
	}

	M_thread_mutex_unlock(ctx->lock);

	if (!count)
		goto end;

	rv = M_TRUE;

end:
	if (sk != NULL)
		sk_X509_INFO_pop_free(sk, X509_INFO_free);
	if (bio != NULL)
		BIO_free(bio);
	return rv;
}


M_bool M_tls_serverctx_add_trust_crl_file(M_tls_serverctx_t *ctx, const char *path)
{
	M_bool         retval;
	unsigned char *crl = NULL;
	size_t         len      = 0;

	if (ctx == NULL || path == NULL)
		return M_FALSE;

	if (M_fs_file_read_bytes(path, 0, &crl, &len) != M_FS_ERROR_SUCCESS)
		return M_FALSE;

	retval = M_tls_serverctx_add_trust_crl(ctx, crl, len);
	M_free(crl);
	return retval;
}


#if OPENSSL_VERSION_NUMBER >= 0x1000200fL
static int M_tls_alpn_cb(SSL *ssl, const unsigned char **out, unsigned char *outlen, const unsigned char *in, unsigned int inlen, void *arg)
{
	/* Return SSL_TLSEXT_ERR_OK or SSL_TLSEXT_ERR_NOACK */
	M_tls_serverctx_t   *ctx    = arg;
	int                  ret    = SSL_TLSEXT_ERR_NOACK;
	unsigned char       *sel    = NULL; /* Why is this not const? docs say SSL_select_next_proto returns a pointer in out not allocated data */
	unsigned char        sellen = 0;

	(void)ssl;

	if (ctx == NULL)
		return SSL_TLSEXT_ERR_NOACK;

	M_thread_mutex_lock(ctx->lock);
	if (ctx->alpn_apps && ctx->alpn_apps_len) {
		if (SSL_select_next_proto(&sel, &sellen, ctx->alpn_apps, (unsigned int)ctx->alpn_apps_len, in, inlen) == OPENSSL_NPN_NEGOTIATED) {
			ret     = SSL_TLSEXT_ERR_OK;
			*out    = sel;
			*outlen = sellen;
		}
	}

	M_thread_mutex_unlock(ctx->lock);

	return ret;
}
#endif


M_bool M_tls_serverctx_set_applications(M_tls_serverctx_t *ctx, M_list_str_t *applications)
{
#if OPENSSL_VERSION_NUMBER < 0x1000200fL
	(void)ctx;
	(void)applications;
	return M_FALSE;
#else
	M_bool  retval = M_TRUE;

	if (ctx == NULL)
		return M_FALSE;

	M_thread_mutex_lock(ctx->lock);

	SSL_CTX_set_alpn_select_cb(ctx->ctx, NULL, NULL);

	if (ctx->alpn_apps) {
		M_free(ctx->alpn_apps);
		ctx->alpn_apps     = NULL;
		ctx->alpn_apps_len = 0;
	}

	ctx->alpn_apps = M_tls_alpn_list(applications, &ctx->alpn_apps_len);
	if (ctx->alpn_apps) {
		SSL_CTX_set_alpn_select_cb(ctx->ctx, M_tls_alpn_cb, ctx);
	}

	M_thread_mutex_unlock(ctx->lock);

	return retval;
#endif
}


/*! Session resumption is enabled by default, can be disabled with this flag */
M_bool M_tls_serverctx_set_session_resumption(M_tls_serverctx_t *ctx, M_bool enable)
{
	if (ctx == NULL)
		return M_FALSE;

	M_thread_mutex_lock(ctx->lock);
	ctx->sessions_enabled = enable;
	M_tls_serverctx_set_session_support(ctx->ctx, enable);

	M_thread_mutex_unlock(ctx->lock);
	return M_TRUE;
}


M_bool M_tls_serverctx_set_negotiation_timeout_ms(M_tls_serverctx_t *ctx, M_uint64 timeout_ms)
{
	if (ctx == NULL || ctx->parent)
		return M_FALSE;

	if (timeout_ms == 0)
		timeout_ms = 10000;

	M_thread_mutex_lock(ctx->lock);
	ctx->negotiation_timeout_ms = timeout_ms;
	M_thread_mutex_unlock(ctx->lock);
	return M_TRUE;
}


char *M_tls_serverctx_get_cipherlist(M_tls_serverctx_t *ctx)
{
	char *ret = NULL;

	if (ctx == NULL)
		return NULL;

	M_thread_mutex_lock(ctx->lock);
	ret = M_tls_ctx_get_cipherlist(ctx->ctx);
	M_thread_mutex_unlock(ctx->lock);

	return ret;
}


static M_bool M_tls_dh_validate(EVP_PKEY *dh)
{
#if OPENSSL_VERSION_NUMBER < 0x3000000fL
	DH *dhtemp  = EVP_PKEY_get0_DH(dh);
	int dhcodes = 0;
	if (dhtemp == NULL)
		return M_FALSE;

	if (DH_check(dhtemp, &dhcodes) != 1 || dhcodes != 0)
		return M_FALSE;

	return M_TRUE;
#else
	EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_from_pkey(NULL, dh, NULL);
	if (EVP_PKEY_get_base_id(dh) != EVP_PKEY_DH || EVP_PKEY_param_check(ctx) > 0)
		return M_TRUE;
	return M_FALSE;
#endif
}


M_bool M_tls_serverctx_set_dhparam(M_tls_serverctx_t *ctx, const unsigned char *dhparam, size_t dhparam_len)
{
	EVP_PKEY *dh      = NULL;
	BIO      *bio     = NULL;
	M_bool    rv      = M_FALSE;

	if (ctx == NULL || (dhparam != NULL && dhparam_len == 0))
		return M_FALSE;

	if (dhparam != NULL) {
		bio = BIO_new_mem_buf(M_CAST_OFF_CONST(void *, dhparam), (int)dhparam_len);
		dh  = PEM_read_bio_Parameters(bio, NULL);
		BIO_free(bio);
		bio = NULL;

		if (dh == NULL || !M_tls_dh_validate(dh))
			goto done;
	}

	M_thread_mutex_lock(ctx->lock);
	if (ctx->dh) {
		EVP_PKEY_free(ctx->dh);
		ctx->dh = NULL;
	}

	/* NOTE: we may set NULL here, this should disable DH */
	ctx->dh = dh;
	dh      = NULL; /* Take ownership */

	M_thread_mutex_unlock(ctx->lock);

	rv = M_TRUE;
done:
	if (bio != NULL)
		BIO_free(bio);
	if (dh != NULL)
		EVP_PKEY_free(dh);

	return rv;
}


M_bool M_tls_serverctx_set_dhparam_file(M_tls_serverctx_t *ctx, const char *dhparam_path)
{
	M_bool         retval;
	unsigned char *dhparams = NULL;
	size_t         len      = 0;

	if (ctx == NULL || dhparam_path == NULL)
		return M_FALSE;

	if (M_fs_file_read_bytes(dhparam_path, 0, &dhparams, &len) != M_FS_ERROR_SUCCESS)
		return M_FALSE;

	retval = M_tls_serverctx_set_dhparam(ctx, dhparams, len);
	M_free(dhparams);
	return retval;
}


