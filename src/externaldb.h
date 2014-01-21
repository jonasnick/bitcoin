#include "util.h"
#include "rpcrawtransaction.cpp"
#include "json/json_spirit_writer_template.h"
#include <hiredis.h>

//TODO: make abstract
class ExternalKeyValueDB 
{
private:
    redisContext *c;
public:
    ExternalKeyValueDB() {
    }
    bool ConnectTo(const char *hostname, int port) {
        struct timeval timeout = { 1, 500000 }; // 1.5 seconds
        c = redisConnectWithTimeout(hostname, port, timeout);
        if (c == NULL || c->err) {
            if (c) {
                LogPrintf("ExternalKeyValueDB: Connection error: %s\n", c->errstr);
                redisFree(c);
            } else {
                LogPrintf("ExternalKeyValueDB: Connection error: can't allocate redis context\n");
            }
            return false;
        }
        return true;
    }

    bool IsSetup() {
        return c!=NULL;
    }

    redisReply *Command(const char *format, ...) {
        redisReply *ret;
        va_list args;
        va_start(args,format);
        ret = (redisReply *)redisvCommand(c,format,args);
        va_end(args);
        return ret;
    }

    bool writeBlockAtHeight(int height, const uint256 *hashBlock) {
        redisReply *setBlockReply = 
                Command("SET blockAtHeight:%i %s",height,hashBlock->ToString().c_str());
        freeReplyObject(setBlockReply);
        return true;
    }

    bool writeTransaction(const CTransaction &tx, const uint256 *hashBlock) {
        json_spirit::Object result;
        TxToJSON(tx, *hashBlock, result);
        string txString = 
            json_spirit::write_string(Value(result),false);

        redisReply *setTxReply = 
            Command("SET tx:%s '%s'",
                        tx.GetHash().ToString().c_str(), 
                        txString.c_str());
        freeReplyObject(setTxReply);
        return true;
    }

    bool deleteTransaction(const CTransaction &tx) {
        redisReply *deleteTxReply = Command("DEL tx:%s",tx.GetHash().ToString().c_str());
        freeReplyObject(deleteTxReply);
        return true;
    }

    bool writeTransactionInput(const CTransaction &tx) {
        BOOST_FOREACH(const CTxIn &in, tx.vin) {
            uint256 inputTxHash = in.prevout.hash;
            redisReply *setTxOutputReply = 
                Command("SADD txInput:%s %s",
                            tx.GetHash().ToString().c_str(),
                            inputTxHash.ToString().c_str()); 
            freeReplyObject(setTxOutputReply);
        }
        return true;
    }

    bool deleteTransactionInput(const CTransaction &tx) {
        BOOST_FOREACH(const CTxIn &in, tx.vin) {
            uint256 inputTxHash = in.prevout.hash;
            redisReply *setTxOutputReply = 
                Command("SREM txInput:%s %s",
                            tx.GetHash().ToString().c_str(),
                            inputTxHash.ToString().c_str()); 
            freeReplyObject(setTxOutputReply);
        }
        return true;
    }


    // for every transactions t_i that is input to the
    // transaction, add member to set
    // output:t_i -> txid for the txid of the
    // new transaction
    // ... 
    // attention: these input transactions might not yet be stored
    // in the database!
    bool writeTransactionOutputs(const CTransaction &tx) {
        BOOST_FOREACH(const CTxIn &in, tx.vin) {
            uint256 inputTxHash = in.prevout.hash;
            redisReply *setTxOutputReply = 
                Command("SADD txOutput:%s %s",
                            inputTxHash.ToString().c_str(), 
                            tx.GetHash().ToString().c_str());
            freeReplyObject(setTxOutputReply);
        }

        return true;
    }

    bool deleteTransactionOutputs(const CTransaction &tx) {
        BOOST_FOREACH(const CTxIn &in, tx.vin) {
            uint256 inputTxHash = in.prevout.hash;
            redisReply *remTxOutputReply = 
                Command("SREM txOutput:%s %s",
                            inputTxHash.ToString().c_str(), 
                            tx.GetHash().ToString().c_str());
            freeReplyObject(remTxOutputReply);
        }
        return true;
    }

    // for every output address in the transaction add new db-entry
    // txUse:outputAddress -> txid
    bool writeAddressTransactions(const CTransaction &tx) {
        BOOST_FOREACH(const CTxOut &out, tx.vout) {
            CTxDestination destinationAddress;
            ExtractDestination(out.scriptPubKey, 
                                destinationAddress);
            CBitcoinAddress address;
            if(address.Set(destinationAddress)) {
                // valid address
                redisReply *setAddressTxReply = 
                    Command("SADD address:%s %s",
                                address.ToString().c_str(), 
                                tx.GetHash().ToString().c_str());
                freeReplyObject(setAddressTxReply);
            }

        // bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, vector<CTxDestination>& addressRet, int& nRequiredRet)
        // maybe p2sh need additional considerations as well
        // multisign transactions need additional considerations:
        // who has redeemed the coins?
        }
        return true;
    }

    bool deleteAddressTransactions(const CTransaction &tx) {
        BOOST_FOREACH(const CTxOut &out, tx.vout) {
            CTxDestination destinationAddress;
            ExtractDestination(out.scriptPubKey, 
                                destinationAddress);
            CBitcoinAddress address;
            if(address.Set(destinationAddress)) {
                // valid address
                redisReply *remAddressTxReply = 
                    Command("SREM address:%s %s",
                                    address.ToString().c_str(), 
                                    tx.GetHash().ToString().c_str());
                freeReplyObject(remAddressTxReply);
            }
        }
        return true;
    }

};
