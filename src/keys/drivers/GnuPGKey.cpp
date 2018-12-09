/*
 *  Copyright (C) 2018 Sven Seeberg <mail@sven-seeberg.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GnuPGKey.h"

/*
 * Generate a list of available keys. The list includes information if+
 * secret key is available or if the secret key is on a smart card.
 */
void GnuPGKey::listPublicKeys() {
gpgme_ctx_t ctx;
gpgme_key_t key;
gpgme_error_t err = gpgme_new (&ctx);

if (!err)
  {
    err = gpgme_op_keylist_start (ctx, "g10code", 0);
    while (!err)
      {
        err = gpgme_op_keylist_next (ctx, &key);
        if (err)
          break;
        printf ("%s:", key->subkeys->keyid);
        if (key->uids && key->uids->name)
          printf (" %s", key->uids->name);
        if (key->uids && key->uids->email)
          printf (" <%s>", key->uids->email);
        putchar ('\n');
        gpgme_key_release (key);
      }
    gpgme_release (ctx);
  }
if (gpg_err_code (err) != GPG_ERR_EOF)
  {
    fprintf (stderr, "can not list keys: %s\n", gpgme_strerror (err));
    exit (1);
  }
}

/*
 * Encrypt the secret with a selected GPG key.
 */
void GnuPGKey::encrypt() {

}

/*
 * Decrypt the secret with a selected GPG key.
 */
void GnuPGKey::decrypt() {

}

/*
 * Return a key that can be used for decrypted.
 */
void GnuPGKey::selectKey() {

}