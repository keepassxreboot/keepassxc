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

#include "OpenPGP.h"

QUuid OpenPGPKey::UUID("d0ba3fa7-e616-420d-9d1a-11089002e866");

/*
 * Encrypt the secret/password with a selected PGP key and store the
 * result in the header.
 */
void OpenPGPKey::addKey() {

}

/*
 * Decrypt the secret with a PGP key.
 */
void OpenPGPKey::decrypt() {

}

/*
 * Return a PGP secret key that is available for decrypting the secret.
 */
void OpenPGPKey::selectKey() {

}

/*
 * Remove the encrypted secret for a specified PGP key.
 */
void OpenPGPKey::deleteKey2() {

}