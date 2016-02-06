/*-
 * Copyright 2016 Vsevolod Stakhov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "config.h"
#include "libcryptobox/keypair.h"
#include "libcryptobox/keypair_private.h"
#include "libutil/str_util.h"
#include "libutil/printf.h"

/**
 * Returns specific private key for different keypair types
 */
static void *
rspamd_cryptobox_keypair_sk (struct rspamd_cryptobox_keypair *kp,
		guint *len)
{
	g_assert (kp != NULL);

	if (kp->alg == RSPAMD_CRYPTOBOX_MODE_25519) {
		if (kp->type == RSPAMD_KEYPAIR_KEX) {
			*len = 32;
			return RSPAMD_CRYPTOBOX_KEYPAIR_25519(kp)->sk;
		}
		else {
			*len = 64;
			return RSPAMD_CRYPTOBOX_KEYPAIR_SIG_25519(kp)->sk;
		}
	}
	else {
		if (kp->type == RSPAMD_KEYPAIR_KEX) {
			*len = 32;
			return RSPAMD_CRYPTOBOX_KEYPAIR_NIST(kp)->sk;
		}
		else {
			*len = 32;
			return RSPAMD_CRYPTOBOX_KEYPAIR_SIG_NIST(kp)->sk;
		}
	}

	/* Not reached */
	return NULL;
}

static void *
rspamd_cryptobox_keypair_pk (struct rspamd_cryptobox_keypair *kp,
		guint *len)
{
	g_assert (kp != NULL);

	if (kp->alg == RSPAMD_CRYPTOBOX_MODE_25519) {
		if (kp->type == RSPAMD_KEYPAIR_KEX) {
			*len = 32;
			return RSPAMD_CRYPTOBOX_KEYPAIR_25519(kp)->pk;
		}
		else {
			*len = 32;
			return RSPAMD_CRYPTOBOX_KEYPAIR_SIG_25519(kp)->pk;
		}
	}
	else {
		if (kp->type == RSPAMD_KEYPAIR_KEX) {
			*len = 65;
			return RSPAMD_CRYPTOBOX_KEYPAIR_NIST(kp)->pk;
		}
		else {
			*len = 65;
			return RSPAMD_CRYPTOBOX_KEYPAIR_SIG_NIST(kp)->pk;
		}
	}

	/* Not reached */
	return NULL;
}

static void *
rspamd_cryptobox_pubkey_pk (struct rspamd_cryptobox_pubkey *kp,
		guint *len)
{
	g_assert (kp != NULL);

	if (kp->alg == RSPAMD_CRYPTOBOX_MODE_25519) {
		if (kp->type == RSPAMD_KEYPAIR_KEX) {
			*len = 32;
			return RSPAMD_CRYPTOBOX_PUBKEY_25519(kp)->pk;
		}
		else {
			*len = 32;
			return RSPAMD_CRYPTOBOX_PUBKEY_SIG_25519(kp)->pk;
		}
	}
	else {
		if (kp->type == RSPAMD_KEYPAIR_KEX) {
			*len = 65;
			return RSPAMD_CRYPTOBOX_PUBKEY_NIST(kp)->pk;
		}
		else {
			*len = 65;
			return RSPAMD_CRYPTOBOX_PUBKEY_SIG_NIST(kp)->pk;
		}
	}

	/* Not reached */
	return NULL;
}

static struct rspamd_cryptobox_keypair *
rspamd_cryptobox_keypair_alloc (enum rspamd_cryptobox_keypair_type type,
		enum rspamd_cryptobox_mode alg)
{
	struct rspamd_cryptobox_keypair *kp;
	guint size = 0;

	if (alg == RSPAMD_CRYPTOBOX_MODE_25519) {
		if (type == RSPAMD_KEYPAIR_KEX) {
			size = sizeof (struct rspamd_cryptobox_keypair_25519);
		}
		else {
			size = sizeof (struct rspamd_cryptobox_keypair_sig_25519);
		}
	}
	else {
		if (type == RSPAMD_KEYPAIR_KEX) {
			size = sizeof (struct rspamd_cryptobox_keypair_nist);
		}
		else {
			size = sizeof (struct rspamd_cryptobox_keypair_sig_nist);
		}
	}

	g_assert (size >= sizeof (*kp));

	if (posix_memalign ((void **)&kp, 32, size) != 0) {
		abort ();
	}

	return kp;
}

static struct rspamd_cryptobox_pubkey *
rspamd_cryptobox_pubkey_alloc (enum rspamd_cryptobox_keypair_type type,
		enum rspamd_cryptobox_mode alg)
{
	struct rspamd_cryptobox_pubkey *kp;
	guint size = 0;

	if (alg == RSPAMD_CRYPTOBOX_MODE_25519) {
		if (type == RSPAMD_KEYPAIR_KEX) {
			size = sizeof (struct rspamd_cryptobox_pubkey_25519);
		}
		else {
			size = sizeof (struct rspamd_cryptobox_pubkey_sig_25519);
		}
	}
	else {
		if (type == RSPAMD_KEYPAIR_KEX) {
			size = sizeof (struct rspamd_cryptobox_pubkey_nist);
		}
		else {
			size = sizeof (struct rspamd_cryptobox_pubkey_sig_nist);
		}
	}

	g_assert (size >= sizeof (*kp));

	if (posix_memalign ((void **)&kp, 32, size) != 0) {
		abort ();
	}

	return kp;
}


void
rspamd_cryptobox_nm_dtor (struct rspamd_cryptobox_nm *nm)
{
	rspamd_explicit_memzero (nm->nm, sizeof (nm->nm));
	free (nm);
}

void
rspamd_cryptobox_keypair_dtor (struct rspamd_cryptobox_keypair *kp)
{
	void *sk;
	guint len = 0;

	sk = rspamd_cryptobox_keypair_sk (kp, &len);
	g_assert (sk != NULL && len > 0);
	rspamd_explicit_memzero (sk, len);
	/* Not g_free as kp is aligned using posix_memalign */
	free (kp);
}

void
rspamd_cryptobox_pubkey_dtor (struct rspamd_cryptobox_pubkey *p)
{
	if (p->nm) {
		REF_RELEASE (p->nm);
	}

	/* Not g_free as p is aligned using posix_memalign */
	free (p);
}

struct rspamd_cryptobox_keypair*
rspamd_keypair_new (enum rspamd_cryptobox_keypair_type type,
		enum rspamd_cryptobox_mode alg)
{
	struct rspamd_cryptobox_keypair *kp;
	void *pk, *sk;
	guint size;

	kp = rspamd_cryptobox_keypair_alloc (type, alg);
	kp->alg = alg;
	kp->type = type;

	sk = rspamd_cryptobox_keypair_sk (kp, &size);
	pk = rspamd_cryptobox_keypair_pk (kp, &size);

	if (type == RSPAMD_KEYPAIR_KEX) {
		rspamd_cryptobox_keypair (pk, sk, alg);
	}
	else {
		rspamd_cryptobox_keypair_sig (pk, sk, alg);
	}

	rspamd_cryptobox_hash (kp->id, pk, size, NULL, 0);

	REF_INIT_RETAIN (kp, rspamd_cryptobox_keypair_dtor);

	return kp;
}


struct rspamd_cryptobox_keypair*
rspamd_keypair_ref (struct rspamd_cryptobox_keypair *kp)
{
	REF_RETAIN (kp);
	return kp;
}


void
rspamd_keypair_unref (struct rspamd_cryptobox_keypair *kp)
{
	REF_RELEASE (kp);
}


struct rspamd_cryptobox_pubkey*
rspamd_pubkey_ref (struct rspamd_cryptobox_pubkey *kp)
{
	REF_RETAIN (kp);
	return kp;
}

void
rspamd_pubkey_unref (struct rspamd_cryptobox_pubkey *kp)
{
	REF_RELEASE (kp);
}

enum rspamd_cryptobox_keypair_type
rspamd_keypair_type (struct rspamd_cryptobox_keypair *kp)
{
	g_assert (kp != NULL);

	return kp->type;
}

enum rspamd_cryptobox_keypair_type
rspamd_pubkey_type (struct rspamd_cryptobox_pubkey *p)
{
	g_assert (p != NULL);

	return p->type;
}


enum rspamd_cryptobox_mode
rspamd_keypair_alg (struct rspamd_cryptobox_keypair *kp)
{
	g_assert (kp != NULL);

	return kp->alg;
}

enum rspamd_cryptobox_mode
rspamd_pubkey_alg (struct rspamd_cryptobox_pubkey *p)
{
	g_assert (p != NULL);

	return p->alg;
}

struct rspamd_cryptobox_pubkey*
rspamd_pubkey_from_base32 (const gchar *b32,
		gsize len,
		enum rspamd_cryptobox_keypair_type type,
		enum rspamd_cryptobox_mode alg)
{
	guchar *decoded;
	gsize dlen, expected_len;
	guint pklen;
	struct rspamd_cryptobox_pubkey *pk;
	guchar *pk_data;

	g_assert (b32 != NULL);

	if (len == 0) {
		len = strlen (b32);
	}

	decoded = rspamd_decode_base32 (b32, len, &dlen);

	if (decoded == NULL) {
		return NULL;
	}

	expected_len = (type == RSPAMD_KEYPAIR_KEX) ?
			rspamd_cryptobox_pk_bytes (alg) : rspamd_cryptobox_pk_sig_bytes (alg);

	if (dlen != expected_len) {
		g_free (decoded);
		return NULL;
	}

	pk = rspamd_cryptobox_pubkey_alloc (type, alg);
	pk_data = rspamd_cryptobox_pubkey_pk (pk, &pklen);

	memcpy (pk_data, decoded, pklen);
	g_free (decoded);
	rspamd_cryptobox_hash (pk->id, pk_data, pklen, NULL, 0);
	pk->alg = alg;
	pk->type = type;

	return pk;
}

struct rspamd_cryptobox_pubkey*
rspamd_pubkey_from_hex (const gchar *hex,
		gsize len,
		enum rspamd_cryptobox_keypair_type type,
		enum rspamd_cryptobox_mode alg)
{
	guchar *decoded;
	gsize dlen, expected_len;
	guint pklen;
	struct rspamd_cryptobox_pubkey *pk;
	guchar *pk_data;

	g_assert (hex != NULL);

	if (len == 0) {
		len = strlen (hex);
	}

	dlen = len / 2;

	decoded = rspamd_decode_hex (hex, len);

	if (decoded == NULL) {
		return NULL;
	}

	expected_len = (type == RSPAMD_KEYPAIR_KEX) ?
			rspamd_cryptobox_pk_bytes (alg) : rspamd_cryptobox_pk_sig_bytes (alg);

	if (dlen != expected_len) {
		g_free (decoded);
		return NULL;
	}

	pk = rspamd_cryptobox_pubkey_alloc (type, alg);
	pk_data = rspamd_cryptobox_pubkey_pk (pk, &pklen);

	memcpy (pk_data, decoded, pklen);
	g_free (decoded);
	rspamd_cryptobox_hash (pk->id, pk_data, pklen, NULL, 0);
	pk->alg = alg;
	pk->type = type;

	return pk;
}

struct rspamd_cryptobox_pubkey*
rspamd_pubkey_from_bin (const guchar *raw,
		gsize len,
		enum rspamd_cryptobox_keypair_type type,
		enum rspamd_cryptobox_mode alg)
{
	gsize expected_len;
	guint pklen;
	struct rspamd_cryptobox_pubkey *pk;
	guchar *pk_data;

	g_assert (raw != NULL && len > 0);

	expected_len = (type == RSPAMD_KEYPAIR_KEX) ?
			rspamd_cryptobox_pk_bytes (alg) : rspamd_cryptobox_pk_sig_bytes (alg);

	if (len != expected_len) {
		return NULL;
	}

	pk = rspamd_cryptobox_pubkey_alloc (type, alg);
	pk_data = rspamd_cryptobox_pubkey_pk (pk, &pklen);

	memcpy (pk_data, raw, pklen);
	rspamd_cryptobox_hash (pk->id, pk_data, pklen, NULL, 0);
	pk->alg = alg;
	pk->type = type;

	return pk;
}


const guchar *
rspamd_pubkey_get_nm (struct rspamd_cryptobox_pubkey *p)
{
	g_assert (p != NULL);

	if (p->nm) {
		return p->nm->nm;
	}

	return NULL;
}

const guchar *
rspamd_pubkey_calculate_nm (struct rspamd_cryptobox_pubkey *p,
		struct rspamd_cryptobox_keypair *kp)
{
	g_assert (kp->alg == p->alg);
	g_assert (kp->type == p->type);
	g_assert (p->type == RSPAMD_KEYPAIR_KEX);

	if (kp->alg == RSPAMD_CRYPTOBOX_MODE_25519) {
		struct rspamd_cryptobox_pubkey_25519 *rk_25519 =
				RSPAMD_CRYPTOBOX_PUBKEY_25519(p);
		struct rspamd_cryptobox_keypair_25519 *sk_25519 =
				RSPAMD_CRYPTOBOX_KEYPAIR_25519(kp);

		rspamd_cryptobox_nm (p->nm->nm, rk_25519->pk, sk_25519->sk, p->alg);
	}
	else {
		struct rspamd_cryptobox_pubkey_nist *rk_nist =
				RSPAMD_CRYPTOBOX_PUBKEY_NIST(p);
		struct rspamd_cryptobox_keypair_nist *sk_nist =
				RSPAMD_CRYPTOBOX_KEYPAIR_NIST(kp);

		rspamd_cryptobox_nm (p->nm->nm, rk_nist->pk, sk_nist->sk, p->alg);
	}

	return p->nm->nm;
}

const guchar *
rspamd_keypair_get_id (struct rspamd_cryptobox_keypair *kp)
{
	g_assert (kp != NULL);

	return kp->id;
}

const guchar *
rspamd_pubkey_get_id (struct rspamd_cryptobox_pubkey *pk)
{
	g_assert (pk != NULL);

	return pk->id;
}

const guchar *
rspamd_pubkey_get_pk (struct rspamd_cryptobox_pubkey *pk,
		guint *len)
{
	guchar *ret = NULL;
	guint rlen;

	ret = rspamd_cryptobox_pubkey_pk (pk, &rlen);

	if (len) {
		*len = rlen;
	}

	return ret;
}

static void
rspamd_keypair_print_component (guchar *data, gsize datalen,
		GString *res, guint how, const gchar *description)
{
	gint olen, b32_len;

	if (how & RSPAMD_KEYPAIR_HUMAN) {
		rspamd_printf_gstring (res, "%s: ", description);
	}

	if (how & RSPAMD_KEYPAIR_BASE32) {
		b32_len = (datalen * 8 / 5) + 2;
		g_string_set_size (res, res->len + b32_len);
		res->len -= b32_len;
		olen = rspamd_encode_base32_buf (data, datalen, res->str + res->len,
				res->len + b32_len - 1);

		if (olen > 0) {
			res->len += olen;
			res->str[res->len] = '\0';
		}
	}
	else if (how & RSPAMD_KEYPAIR_HEX) {
		rspamd_printf_gstring (res, "%*xs", (gint)datalen, data);
	}
	else {
		g_string_append_len (res, data, datalen);
	}

	if (how & RSPAMD_KEYPAIR_HUMAN) {
		g_string_append_c (res, '\n');
	}
}

GString *
rspamd_keypair_print (struct rspamd_cryptobox_keypair *kp, guint how)
{
	GString *res;
	guint len;
	gpointer p;

	g_assert (kp != NULL);

	res = g_string_sized_new (63);

	if ((how & RSPAMD_KEYPAIR_PUBKEY)) {
		p = rspamd_cryptobox_keypair_pk (kp, &len);
		rspamd_keypair_print_component (p, len, res, how, "Public key");
	}
	if ((how & RSPAMD_KEYPAIR_PRIVKEY)) {
		p = rspamd_cryptobox_keypair_sk (kp, &len);
		rspamd_keypair_print_component (p, len, res, how, "Private key");
	}
	if ((how & RSPAMD_KEYPAIR_ID_SHORT)) {
		rspamd_keypair_print_component (kp->id, 5, res, how, "Short key ID");
	}
	if ((how & RSPAMD_KEYPAIR_ID)) {
		rspamd_keypair_print_component (kp->id, sizeof (kp->id), res, how, "Key ID");
	}

	return res;
}

const guchar *
rspamd_keypair_component (struct rspamd_cryptobox_keypair *kp,
		guint ncomp, guint *len)
{
	guint rlen = 0;
	const guchar *ret = NULL;

	g_assert (kp != NULL);

	switch (ncomp) {
	case RSPAMD_KEYPAIR_COMPONENT_ID:
		rlen = sizeof (kp->id);
		ret = kp->id;
		break;
	case RSPAMD_KEYPAIR_COMPONENT_PK:
		ret = rspamd_cryptobox_keypair_pk (kp, &rlen);
		break;
	case RSPAMD_KEYPAIR_COMPONENT_SK:
		ret = rspamd_cryptobox_keypair_sk (kp, &rlen);
		break;
	}

	if (len) {
		*len = rlen;
	}

	return ret;
}

struct rspamd_cryptobox_keypair *
rspamd_keypair_from_ucl (const ucl_object_t *obj)
{
	const ucl_object_t *privkey, *pubkey, *elt;
	const gchar *str;
	enum rspamd_cryptobox_keypair_type type = RSPAMD_KEYPAIR_KEX;
	enum rspamd_cryptobox_mode mode = RSPAMD_CRYPTOBOX_MODE_25519;
	gboolean is_hex = FALSE;
	struct rspamd_cryptobox_keypair *kp;
	guint len;
	gsize ucl_len;
	gint dec_len;
	gpointer target;

	if (ucl_object_type (obj) != UCL_OBJECT) {
		return NULL;
	}

	elt = ucl_object_find_key (obj, "keypair");
	if (elt != NULL) {
		obj = elt;
	}

	pubkey = ucl_object_find_any_key (obj, "pubkey", "public", "public_key",
			NULL);
	if (pubkey == NULL || ucl_object_type (pubkey) != UCL_STRING) {
		return NULL;
	}

	privkey = ucl_object_find_any_key (obj, "privkey", "private", "private_key",
			"secret", "secret_key", NULL);
	if (privkey == NULL || ucl_object_type (privkey) != UCL_STRING) {
		return NULL;
	}

	/* Optional fields */
	elt = ucl_object_find_key (obj, "type");
	if (elt && ucl_object_type (elt) == UCL_STRING) {
		str = ucl_object_tostring (elt);

		if (g_ascii_strcasecmp (str, "kex") == 0) {
			type = RSPAMD_KEYPAIR_KEX;
		}
		else if (g_ascii_strcasecmp (str, "sign") == 0) {
			type = RSPAMD_KEYPAIR_SIGN;
		}
		/* TODO: handle errors */
	}

	elt = ucl_object_find_key (obj, "algorithm");
	if (elt && ucl_object_type (elt) == UCL_STRING) {
		str = ucl_object_tostring (elt);

		if (g_ascii_strcasecmp (str, "curve25519") == 0) {
			mode = RSPAMD_CRYPTOBOX_MODE_25519;
		}
		else if (g_ascii_strcasecmp (str, "nistp256") == 0) {
			mode = RSPAMD_CRYPTOBOX_MODE_NIST;
		}
		/* TODO: handle errors */
	}

	elt = ucl_object_find_key (obj, "encoding");
	if (elt && ucl_object_type (elt) == UCL_STRING) {
		str = ucl_object_tostring (elt);

		if (g_ascii_strcasecmp (str, "hex") == 0) {
			is_hex = TRUE;
		}
		/* TODO: handle errors */
	}

	kp = rspamd_keypair_new (type, mode);
	g_assert (kp != NULL);

	target = rspamd_cryptobox_keypair_pk (kp, &len);
	str = ucl_object_tolstring (pubkey, &ucl_len);

	if (is_hex) {
		dec_len = rspamd_decode_hex_buf (str, ucl_len, target, len);
	}
	else {
		dec_len = rspamd_decode_base32_buf (str, ucl_len, target, len);
	}

	if (dec_len != (gint)len) {
		rspamd_keypair_unref (kp);

		return NULL;
	}

	target = rspamd_cryptobox_keypair_sk (kp, &len);
	str = ucl_object_tolstring (privkey, &ucl_len);

	if (is_hex) {
		dec_len = rspamd_decode_hex_buf (str, ucl_len, target, len);
	}
	else {
		dec_len = rspamd_decode_base32_buf (str, ucl_len, target, len);
	}

	if (dec_len != (gint)len) {
		rspamd_keypair_unref (kp);

		return NULL;
	}

	return kp;
}

ucl_object_t *
rspamd_keypair_to_ucl (struct rspamd_cryptobox_keypair *kp,
		gboolean is_hex)
{
	ucl_object_t *ucl_out, *elt;
	gint how = 0;
	GString *keypair_out;
	const gchar *encoding;

	g_assert (kp != NULL);

	if (is_hex) {
		how |= RSPAMD_KEYPAIR_HEX;
		encoding = "hex";
	}
	else {
		how |= RSPAMD_KEYPAIR_BASE32;
		encoding = "base32";
	}

	ucl_out = ucl_object_typed_new (UCL_OBJECT);
	elt = ucl_object_typed_new (UCL_OBJECT);
	ucl_object_insert_key (ucl_out, elt, "keypair", 0, false);

	/* pubkey part */
	keypair_out = rspamd_keypair_print (kp,
			RSPAMD_KEYPAIR_PUBKEY|how);
	ucl_object_insert_key (elt,
			ucl_object_fromlstring (keypair_out->str, keypair_out->len),
			"pubkey", 0, false);
	g_string_free (keypair_out, TRUE);

	/* privkey part */
	keypair_out = rspamd_keypair_print (kp,
			RSPAMD_KEYPAIR_PRIVKEY|how);
	ucl_object_insert_key (elt,
			ucl_object_fromlstring (keypair_out->str, keypair_out->len),
			"privkey", 0, false);
	g_string_free (keypair_out, TRUE);

	keypair_out = rspamd_keypair_print (kp,
			RSPAMD_KEYPAIR_ID|how);
	ucl_object_insert_key (elt,
			ucl_object_fromlstring (keypair_out->str, keypair_out->len),
			"id", 0, false);
	g_string_free (keypair_out, TRUE);

	ucl_object_insert_key (elt,
			ucl_object_fromstring (encoding),
			"encoding", 0, false);

	ucl_object_insert_key (elt,
			ucl_object_fromstring (
					kp->alg == RSPAMD_CRYPTOBOX_MODE_NIST ?
							"nistp256" : "curve25519"),
					"algorithm", 0, false);

	ucl_object_insert_key (elt,
			ucl_object_fromstring (
					kp->type == RSPAMD_KEYPAIR_KEX ?
							"kex" : "sign"),
					"type", 0, false);

	return ucl_out;
}