// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "consensus/merkle.h"
#include "primitives/block.h"
#include "core_io.h"
#include "script/script.h"
#include "script/standard.h"
#include "script/interpreter.h"
#include "addrman.h"
#include "chain.h"
#include "coins.h"
#include "compressor.h"
#include "net.h"
#include "protocol.h"
#include "streams.h"
#include "undo.h"
#include "version.h"
#include "util.cpp"

#include <stdint.h>
#include <unistd.h>
#include <univalue.h>

#include <algorithm>
#include <vector>

#define SCRIPTCODELEN 1

bool read_stdin(std::vector<char> &data) {
    char buffer[1024];
    ssize_t length=0;
    while((length = read(STDIN_FILENO, buffer, 1024)) > 0) {
        data.insert(data.end(), buffer, buffer+length);
        if (data.size() > (1<<20)) return false;
    }
    return length==0;
}

UniValue ValueFromAmount(const CAmount& amount)
{
    bool sign = amount < 0;
    int64_t n_abs = (sign ? -amount : amount);
    int64_t quotient = n_abs / COIN;
    int64_t remainder = n_abs % COIN;
    return UniValue(UniValue::VNUM,
            strprintf("%s%d.%08d", sign ? "-" : "", quotient, remainder));
}

void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry)
{
    entry.push_back(Pair("txid", tx.GetHash().GetHex()));
    entry.push_back(Pair("size", (int)::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_WITNESS)));
    entry.push_back(Pair("version", tx.nVersion));
    entry.push_back(Pair("locktime", (int64_t)tx.nLockTime));
    UniValue vin(UniValue::VARR);
    BOOST_FOREACH(const CTxIn& txin, tx.vin) {
        UniValue in(UniValue::VOBJ);
        if (tx.IsCoinBase())
            in.push_back(Pair("coinbase", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
        else {
            in.push_back(Pair("txid", txin.prevout.hash.GetHex()));
            in.push_back(Pair("vout", (int64_t)txin.prevout.n));
            UniValue o(UniValue::VOBJ);
            o.push_back(Pair("asm", ScriptToAsmStr(txin.scriptSig, true)));
            o.push_back(Pair("hex", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
            in.push_back(Pair("scriptSig", o));
        }
        in.push_back(Pair("sequence", (int64_t)txin.nSequence));
        vin.push_back(in);
    }
    entry.push_back(Pair("vin", vin));
    UniValue vout(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& txout = tx.vout[i];
        UniValue out(UniValue::VOBJ);
        out.push_back(Pair("value", ValueFromAmount(txout.nValue)));
        out.push_back(Pair("n", (int64_t)i));
        UniValue o(UniValue::VOBJ);
        o.push_back(Pair("asm", ScriptToAsmStr(txout.scriptPubKey, true)));
        o.push_back(Pair("hex", HexStr(txout.scriptPubKey.begin(), txout.scriptPubKey.end())));
        out.push_back(Pair("scriptPubKey", o));
        vout.push_back(out);
    }
    entry.push_back(Pair("vout", vout));

}


void createSeed() {
    int nVersion = (PROTOCOL_VERSION | SERIALIZE_TRANSACTION_WITNESS) & ~SERIALIZE_TRANSACTION_NO_WITNESS;

    std::vector<char> buffer;
    CDataStream ds(buffer, SER_NETWORK, nVersion);

    CMutableTransaction txTo1;
    txTo1.nVersion = 1337;
    std::vector<CTxIn> vins;
    vins.push_back(CTxIn());
    vins.push_back(CTxIn());
    txTo1.vin = vins;
    std::vector<CTxOut> vouts;
    vouts.push_back(CTxOut());
    vouts.push_back(CTxOut());
    txTo1.vout = vouts;
    txTo1.nLockTime = 100;

    ds << CTransaction(txTo1);

    txTo1.nVersion = 1338;
    ds << CTransaction(txTo1);

    CScript scriptCode = CScript() << OP_2;
    //std::vector<unsigned char> scriptCodeVector(SCRIPTCODELEN);
    ds.write((char*)&scriptCode[0], SCRIPTCODELEN);
    ds.write((char*)&scriptCode[0], SCRIPTCODELEN);

    int nHashType = SIGHASH_ALL | SIGHASH_SINGLE | SIGHASH_NONE | SIGHASH_ANYONECANPAY;
    ds << nHashType;

    std::ofstream myFile;
    myFile.open ("data.bin", ios::out | ios::binary);
    ds.Serialize(myFile, 0, 0);
    myFile.close();
}
void print(string dir, CTransaction& txTo1, CTransaction& txTo2, CScript& scriptCode1, CScript& scriptCode2, int nHashType) {
    UniValue result(UniValue::VOBJ);
    uint256 blockHash;

    ofstream sigHash1File;
    sigHash1File.open(dir + "/sigHash1.txt");
    ofstream sigHash2File;
    sigHash2File.open(dir + "/sigHash2.txt");

    sigHash1File << "======================================================" << "\n";
    sigHash1File << "Tx 1" << "\n";
    sigHash1File << "======================================================" << "\n";
    TxToJSON(txTo1, blockHash, result);
    sigHash1File << result.write(1,1) << '\n';

    sigHash2File << "\n";
    sigHash2File << "======================================================" << "\n";
    sigHash2File << "Tx 2" << "\n";
    sigHash2File << "======================================================" << "\n";

    result = UniValue(UniValue::VOBJ);
    TxToJSON(txTo2, blockHash, result);
    sigHash2File << result.write(1,1) << '\n';

    sigHash1File << "\n";
    sigHash1File << "======================================================" << "\n";
    sigHash1File << "scriptCode1" << "\n";
    sigHash1File << "======================================================" << "\n";
    sigHash1File << ScriptToAsmStr(scriptCode1) << '\n';

    sigHash2File << "\n";
    sigHash2File << "======================================================" << "\n";
    sigHash2File << "scriptCode2" << "\n";
    sigHash2File << "======================================================" << "\n";
    sigHash2File << ScriptToAsmStr(scriptCode2) << '\n';

    sigHash1File << "\n";
    sigHash1File << "======================================================" << "\n";
    sigHash1File << "hash type" << "\n";
    sigHash1File << "======================================================" << "\n";

    sigHash2File << "\n";
    sigHash2File << "======================================================" << "\n";
    sigHash2File << "hash type" << "\n";
    sigHash2File << "======================================================" << "\n";
    if (nHashType & SIGHASH_ALL) {
        sigHash1File << "SIGHASH_ALL" << "|";
        sigHash2File << "SIGHASH_ALL" << "|";
    }
    if (nHashType & SIGHASH_NONE) {
        sigHash1File << "SIGHASH_NONE" << "|";
        sigHash2File << "SIGHASH_NONE" << "|";
    }
    if (nHashType & SIGHASH_SINGLE) {
        sigHash1File << "SIGHASH_SINGLE" << "|";
        sigHash2File << "SIGHASH_SINGLE" << "|";
    }
    if (nHashType & SIGHASH_ANYONECANPAY) {
        sigHash1File << "SIGHASH_ANYONECANPAY" << "|";
        sigHash2File << "SIGHASH_ANYONECANPAY" << "|";
    }
    sigHash1File << "\n";
    sigHash2File << "\n";
    sigHash1File.close();
    sigHash2File.close();
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "seed") == 0) {
        createSeed();
        return 0;
    }
    std::vector<char> buffer;
    if (!read_stdin(buffer)) return 0;

    if (buffer.size() < sizeof(uint32_t)) return 0;

    int nVersion = (PROTOCOL_VERSION | SERIALIZE_TRANSACTION_WITNESS) & ~SERIALIZE_TRANSACTION_NO_WITNESS;

    CDataStream ds(buffer, SER_NETWORK, nVersion);

    //CDataStream ds(buffer, SER_NETWORK, PROTOCOL_VERSION);
    // SERIALIZE_TRANSACTION_WITNESS
/*    try {*/
        //ds.ReadVersion();
    //} catch (const std::ios_base::failure& e) {
        //return 0;
    /*}*/

    CTransaction txTo1;
    try
    {
        ds >> txTo1;
    } catch (const std::ios_base::failure& e) {return 0;}

    CTransaction txTo2;
    try
    {
        ds >> txTo2;
    } catch (const std::ios_base::failure& e) {return 0;}

    CScript scriptCode1;
    try
    {
        char pch[SCRIPTCODELEN];
        ds.read(&pch[0], SCRIPTCODELEN);
        scriptCode1 = CScript((unsigned char*) &pch[0], (unsigned char*) &pch[SCRIPTCODELEN]);
    } catch (const std::ios_base::failure& e) {return 0;}

    CScript scriptCode2;
    try
    {
        char pch[SCRIPTCODELEN];
        ds.read(&pch[0], SCRIPTCODELEN);
        scriptCode2 = CScript((unsigned char*) &pch[0], (unsigned char*) &pch[SCRIPTCODELEN]);

    } catch (const std::ios_base::failure& e) {return 0;}

    int nHashType;
    try
    {
        ds >> nHashType;
    } catch (const std::ios_base::failure& e) {return 0;}

    int nIn = 0;
    if (txTo1.vin.size() <= nIn || txTo2.vin.size() <= nIn) {
        return 0;
    }

    // avoid sighash single bug
    if ((nHashType & 0x1f) == SIGHASH_SINGLE && nIn >= txTo1.vout.size()) {
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "print") == 0) {
        if (argc > 2 ) {
            print(argv[2], txTo1, txTo2, scriptCode1, scriptCode2, nHashType);
        } else {
            cout << "no output dir given" << '\n';
        }
    }
    CAmount amount(0);
    uint256 s0 = SignatureHash(scriptCode1, txTo1, nIn, nHashType, amount, SIGVERSION_BASE);
    uint256 s1 = SignatureHash(scriptCode2, txTo2, nIn, nHashType, amount, SIGVERSION_BASE);
    uint256 s2 = SignatureHash(scriptCode1, txTo1, nIn, nHashType, amount, SIGVERSION_WITNESS_V0);
    uint256 s3 = SignatureHash(scriptCode2, txTo2, nIn, nHashType, amount, SIGVERSION_WITNESS_V0);
    if (s0 == s1 && s2 != s3) {
        if (argc > 2 ) {
            ofstream result;
            result.open(string(argv[2]) + "/result.txt");
            result << "h(tx1) == h(tx2) && h'(tx1) != h'(tx2)" << '\n';
            result.close();
        }
        assert(false);
    } else if (s0 != s1 && s2 == s3) {
        if (argc > 2 ) {
            ofstream result;
            result.open(string(argv[2]) + "/result.txt");
            result << "h(tx1) != h(tx2) && h'(tx1) == h'(tx2)" << '\n';
            result.close();
        }
        assert(false);
    } else {
        cout << "no collision" << '\n';
    }

    return 0;
}

