import hashlib
import os
import time

from . import six
from .exceptions import IrodsError, IrodsWarning

seq_list = [
        0xd768b678,
        0xedfdaf56,
        0x2420231b,
        0x987098d8,
        0xc1bdfeee,
        0xf572341f,
        0x478def3a,
        0xa830d343,
        0x774dfa2a,
        0x6720731e,
        0x346fa320,
        0x6ffdf43a,
        0x7723a320,
        0xdf67d02e,
        0x86ad240a,
        0xe76d342e
    ]

#Don't forget to drink your Ovaltine
wheel = [
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '!', '"', '#', '$', '%', '&', "'", '(', ')', '*', '+', ',', '-', '.', '/'
    ]

default_password_key = 'a9_3fker'
default_scramble_prefix = '.E_'

#Decode a password from a .irodsA file
def decode(s, uid=None):
    #This value lets us know which seq value to use
    #Referred to as "rval" in the C code
    seq_index = ord(s[6]) - ord('e')
    seq = seq_list[seq_index]

    #How much we bitshift seq by when we use it
    #Referred to as "addin_i" in the C code
    #Since we're skipping five bytes that are normally read,
    #we start at 15
    bitshift = 15

    #The uid is used as a salt.
    if uid is None:
        uid = os.getuid()

    #The first byte is a dot, the next five are literally irrelevant
    #garbage, and we already used the seventh one. The string to decode
    #starts at byte eight.
    encoded_string = s[7:]

    decoded_string = ''

    for c in encoded_string:
        if ord(c) == 0:
            break
        #How far this character is from the target character in wheel
        #Referred to as "add_in" in the C code
        offset = ((seq >> bitshift) & 0x1f) + (uid & 0xf5f)

        bitshift += 3
        if bitshift > 28:
            bitshift = 0

        #The character is only encoded if it's one of the ones in wheel
        if c in wheel:
            #index of the target character in wheel
            wheel_index = (len(wheel) + wheel.index(c) - offset) % len(wheel)
            decoded_string += wheel[wheel_index]
        else:
            decoded_string += c

    return decoded_string

#encode passwords to store in the .irodsA file
def encode(s, uid=None, mtime=None):
    #mtime & 65535 needs to be within 20 seconds of the
    #.irodsA file's mtime & 65535
    if mtime is None:
        mtime = int(time.time())

    #How much we bitshift seq by when we use it
    #Referred to as "addin_i" in the C code
    #We can't skip the first five bytes this time,
    #so we start at 0
    bitshift = 0

    #The uid is used as a salt.
    if uid is None:
        uid = os.getuid()

    #This value lets us know which seq value to use
    #Referred to as "rval" in the C code
    #The C code is very specific about this being mtime & 15,
    #but it's never checked. Let's use zero.
    seq_index = 0
    seq = seq_list[seq_index]

    to_encode = ''

    #The C code DOES really care about this value matching
    #the seq_index, though
    to_encode += chr(ord('S') - ((seq_index & 0x7) * 2))

    #And this is also a song and dance to
    #convince the C code we are legitimate
    to_encode += chr(((mtime >> 4) & 0xf) + ord('a'))
    to_encode += chr((mtime & 0xf) + ord('a'))
    to_encode += chr(((mtime >> 12) & 0xf) + ord('a'))
    to_encode += chr(((mtime >> 8) & 0xf) + ord('a'))

    #We also want to actually encode the passed string
    to_encode += s

    #Yeah, the string starts with a dot. Whatever.
    encoded_string = '.'

    for c in to_encode:
        if ord(c) == 0:
            break

        #How far this character is from the target character in wheel
        #Referred to as "add_in" in the C code
        offset = ((seq >> bitshift) & 0x1f) + (uid & 0xf5f)

        bitshift += 3
        if bitshift > 28:
            bitshift = 0

        #The character is only encoded if it's one of the ones in wheel
        if c in wheel:
            #index of the target character in wheel
            wheel_index = (wheel.index(c) + offset) % len(wheel)
            encoded_string += wheel[wheel_index]
        else:
            encoded_string += c

    #insert the seq_index (which is NOT encoded):
    encoded_string = chr(seq_index + ord('e')).join([
            encoded_string[:6],
            encoded_string[6:]
        ])

    #aaaaand, append a null character. because we want to print
    #a null character to the file. because that's a good idea.
    encoded_string += chr(0)

    return encoded_string

#Hash some stuff to create ANOTHER encoder ring.
def get_encoder_ring(key=default_password_key):

    md5_hasher = hashlib.md5()
    #key (called keyBuf in the C) is padded
    #or truncated to 100 characters
    md5_hasher.update(key.ljust(100, chr(0))[:100].encode('ascii'))
    first = md5_hasher.digest()

    md5_hasher = hashlib.md5()
    md5_hasher.update(first)
    second = md5_hasher.digest()

    md5_hasher = hashlib.md5()
    md5_hasher.update(first + second)
    third = md5_hasher.digest()

    return first + second + third + third

#unscramble passwords stored in the database
def unscramble(s, key=default_password_key, scramble_prefix=default_scramble_prefix, block_chaining=False):
    if key is None:
        key=default_password_key

    if not s.startswith(scramble_prefix):
        #not scrambled. or if it is, not
        #in a way we can unscramble
        return s

    #the prefix is nonsense, strip it off
    to_unscramble = s[len(scramble_prefix):]

    #get our encoder ring (called cpKey in the C code)
    encoder_ring = get_encoder_ring(key)
    encoder_ring_index = 0

    #for block chaining
    chain = 0

    unscrambled_string = ''
    for c in to_unscramble:
        if c in wheel:
            #the index of the target character in wheel
            wheel_index = (wheel.index(c) - six.indexbytes(encoder_ring, encoder_ring_index % 61) - chain) % len(wheel)
            unscrambled_string += wheel[wheel_index]
            if block_chaining:
                chain = ord(c) & 0xff
        else:
            unscrambled_string += c
        encoder_ring_index += 1

    return unscrambled_string

#scramble passwords to store in the database
def scramble(s, key=default_password_key, scramble_prefix=default_scramble_prefix, block_chaining=False):
    if key is None:
        key=default_password_key

    to_scramble = s

    #get our encoder ring (called cpKey in the C code)
    encoder_ring = get_encoder_ring(key)
    encoder_ring_index = 0

    #for block chaining
    chain = 0

    scrambled_string = ''
    for c in to_scramble:
        if c in wheel:
            #the index of the target character in wheel
            wheel_index = (wheel.index(c) + six.indexbytes(encoder_ring, encoder_ring_index % 61) + chain) % len(wheel)
            scrambled_string += wheel[wheel_index]
            if block_chaining:
                chain = ord(scrambled_string[-1]) & 0xff
        else:
            scrambled_string += c
        encoder_ring_index += 1

    return scramble_prefix + scrambled_string
