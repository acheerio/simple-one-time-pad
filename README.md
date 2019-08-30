# simple-one-time-pad
Simple one-time pad with encrypt and decrypt daemons

Steps:
1. Compile: compileall
2. Run encryption daemon as background process: otp_enc_d <port_num1> &
3. Run decryption daemon as background process: otp_dec_d <port_num2> &
4. To generate a key file <key_filename>: keygen <number_of_characters> > <key_filename>
5. To establish client connection for encryption: otp_enc <plaintext_filename> <key_filename> <port_num1>
6. To establish client connection for decryption: otp_dec <ciphertext_filename> <key_filename> <port_num2>

Coded in and created on Linux flip1.engr.oregonstate.edu 3.10.0-862.14.4.el7.x86_64
