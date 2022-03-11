package main

import (
	"bytes"
	"crypto/hmac"
	"crypto/sha1"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"syscall"

	"encoding/binary"
	"encoding/hex"

	"golang.org/x/crypto/ssh/terminal"
)

const fileVersionCriticalMask uint32 = 0xFFFF0000
const argon2Salt = "S"
const endOfHeader = 0
const endOfVariantMap = 0
const kdfParameters = 11

func readSecret() (string, error) {
	fmt.Print("Secret: ")
	byteSecret, err := terminal.ReadPassword(int(syscall.Stdin))
	fmt.Println()
	secret := string(byteSecret)
	return secret, err

}
func readHeaderField(reader io.Reader) (bool, byte, []byte, error) {
	var fieldID byte
	err := binary.Read(reader, binary.LittleEndian, &fieldID)
	if err != nil {
		return true, 0, nil, err
	}

	if fieldID == endOfHeader {
		return false, 0, nil, nil
	}

	var fieldLength uint32
	err = binary.Read(reader, binary.LittleEndian, &fieldLength)
	if err != nil {
		return true, fieldID, nil, err
	}

	fieldData := make([]byte, fieldLength)
	err = binary.Read(reader, binary.LittleEndian, &fieldData)
	if err != nil {
		return true, fieldID, fieldData, err
	}
	return true, fieldID, fieldData, nil
}
func readVariantMap(reader io.Reader) ([]byte, error) {
	var version uint16
	err := binary.Read(reader, binary.LittleEndian, &version)
	if err != nil {
		return nil, err
	}

	var fieldType byte
	for err = binary.Read(reader, binary.LittleEndian, &fieldType); fieldType != endOfVariantMap && err == nil; err = binary.Read(reader, binary.LittleEndian, &fieldType) {

		var nameLen uint32
		err = binary.Read(reader, binary.LittleEndian, &nameLen)
		if err != nil {
			return nil, err
		}

		nameBytes := make([]byte, nameLen)
		err = binary.Read(reader, binary.LittleEndian, &nameBytes)
		if err != nil {
			return nil, err
		}

		name := string(nameBytes)

		var valueLen uint32
		err = binary.Read(reader, binary.LittleEndian, &valueLen)
		if err != nil {
			return nil, err
		}

		value := make([]byte, valueLen)
		err = binary.Read(reader, binary.LittleEndian, &value)
		if err != nil {
			return nil, err
		}

		if name == argon2Salt {
			return value, nil
		}
	}
	return nil, nil
}
func readKeepassHeader(keepassFilename string) ([]byte, error) {
	dbFile, err := os.Open(keepassFilename)
	defer dbFile.Close()
	if err != nil {
		return nil, err
	}

	var sig1, sig2, version uint32
	err = binary.Read(dbFile, binary.LittleEndian, &sig1)
	if err != nil {
		return nil, err
	}

	err = binary.Read(dbFile, binary.LittleEndian, &sig2)
	if err != nil {
		return nil, err
	}

	err = binary.Read(dbFile, binary.LittleEndian, &version)
	if err != nil {
		return nil, err
	}

	version &= fileVersionCriticalMask

	var fieldData []byte
	var fieldID byte
	var moreFields bool

	for moreFields, fieldID, fieldData, err = readHeaderField(dbFile); moreFields && err == nil && fieldID != kdfParameters; moreFields, fieldID, fieldData, err = readHeaderField(dbFile) {
	}
	if err != nil {
		return nil, err
	}

	fieldReader := bytes.NewReader(fieldData)
	seed, err := readVariantMap(fieldReader)
	if err != nil {
		return nil, err
	}
	return seed, nil

}
func main() {
	log.SetFlags(0)
	args := os.Args

	if len(args) != 3 {
		log.Fatalf("usage: %s keepassxc-database keyfile", args[0])
	}

	dbFilename := args[1]
	keyFilename := args[2]

	if _, err := os.Stat(keyFilename); err == nil {
		log.Fatalf("keyfile already exists, exiting")
	}
	secretHex, err := readSecret()
	if err != nil {
		log.Fatalf("couldn't read secret from stdin: %s", err)
	}
	secret, err := hex.DecodeString(secretHex)

	if err != nil {
		log.Fatalf("couldn't decode secret: %s", err)
	}

	challenge, err := readKeepassHeader(dbFilename)
	if err != nil {
		log.Fatalf("couldn't read challenge: %s", err)
	}

	if len(challenge) < 64 {
		padd := make([]byte, 64-len(challenge))
		for i, _ := range padd {
			padd[i] = byte(64-len(challenge))
		}
		challenge = append(challenge[:], padd[:]...)
	}

	mac := hmac.New(sha1.New, secret)
	mac.Write(challenge)

	hash := mac.Sum(nil)

	err = ioutil.WriteFile(keyFilename, hash, 0644)
	if err != nil {
		log.Fatalf("couldn't write keyfile: %s", err)
	}

}
