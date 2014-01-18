/*
 * TODO:
 * where are transactions stored?
 * when is the active chain changed, listener on that?
 *      recognize reorg
 *      reorg process
 *          get hash of newest block
 *          find common ancestor of db and bitcoind chain
 *          remove db chain children
 *          add new chain until we are at common ancestor
 * 
 * how to start the bitcoind such that a chain is built?
 */



/*
 * in memory chain of blocks
 */
CChain chainActive //= vector<CBlockIndex*>

ProcessMessage & LoadExternalBlockFile {
    ProcessBlock {
        AcceptBlock {
            AddToBlockIndex {
                ConnectBestBlock {
                    SetBestChain {
                        ConnectBlock {
                            CheckInputs {
                            }
                        }
                    }
                }
            }
        }
    }
}

VerifyDB {
    ConnectBlock {
        CheckInputs {
        }
    }
}


AcceptToMemoryPool {
    CheckInputs
}

bool SetBestChain(CValidationState &state, CBlockIndex* pindexNew) {
    //
    chainActive.SetTip(pindexNew);
    //
}


bool static LoadBlockIndexDB() {
    chainActive.SetTip(it->second);
}

void UnloadBlockIndex()


/*
 * levelDb of unspent output for validation
 * chainstate/
 */
CCoinsViewDB *pcoinsTip

/*
 * leveldb contains metadata of known blocks and
 * where to find them on disk.
 * blocks/index/
 */
CBlockTreeDB //LevelDB

/*
 * blocks/blk.dat 
 * actual bitcoin blocks in network format.
 * only thing that is non-redundant
 * needed for re-scanning missing transactions, reorganizing and serving the blockchain to other nodes
 */

/*
 * blocks/rev*.dat contains undo data
 */

AcceptBlock
    AddToBlockIndex

// CChain implementation
CBlockIndex *CChain::SetTip(CBlockIndex *pindex) {
    if (pindex == NULL) {
        vChain.clear();
        return NULL;
    }
    vChain.resize(pindex->nHeight + 1);
    while (pindex && vChain[pindex->nHeight] != pindex) {
        vChain[pindex->nHeight] = pindex;
        pindex = pindex->pprev;
    }
    return pindex;
}

CBlockIndex *CChain::SetTip(CBlockIndex *pindex) {
    if (pindex == NULL) {
        vChain.clear();
        return NULL;
    }
    vChain.resize(pindex->nHeight + 1);
    while (pindex && vChain[pindex->nHeight] != pindex) {
        vChain[pindex->nHeight] = pindex;
        pindex = pindex->pprev;
    }
    return pindex;
}

//pseudocode
//check return values
//omit getBlockFromDisk by supplying the blocks (or one of them as arguments)
//
//LoadBlockIndex does not get transactions nor the corresponding blocks
//ConnectBlock (called by VerifyDB) f.e. checks transactions using CheckInputs
//but ConnectBlock is used to care about our unspent transactions
void customDB::SetTip(CBlockIndex *pindex) {
    //key-value database
    KVdb db = ...;

    while(pindex && db.get["blockchain:"+pindex->nHeight] !=  pindex->phashBlock) {

        if(db.get["blockchain:"+pindex->nHeight] != NULL) {
            //if there is something to remove, its not just a new block on top of the chain
            CBlock bOld; 
            //old block should still be on disk
            ReadBlockFromDisk(bOld, 
                    mapBlockIndex.find(db.get["blockchain:"+pindex->nHeight]));
            std::vector<CTransactions> transactionsOld = bOld.vtx;

            //remove old block 
            db.set("blockchain:"+pindex->nHeight) = pindex->phashBlock;

            //remove transactions of old block
            for(txid in transactionsOld) {
                for fromToPair in transactionsOld { 
                    //from to pair needs bitcoins GetTransaction to get previous transaction?

                    //and parse CScripts necessary
                    //(find appropriate method to do that parsing) 
                    db.srem("txid:"+fromAddress+toAddress, txid);
                    if(db.count("txid:"+fromAddress+toAddress) == 0) {
                        db.del("from:"+fromAddress);
                        db.del("to:"+toAddress);
                        db.del("txid:"+fromAddress+toAddress);
                    }
                }
                db.del("from:to:"+txid);
            }
        }

        CBlock bNew; 
        ReadBlockFromDisk(bNew, pindex);
        std::vector<CTransactions> transactionsNew = bNew.vtx;

        //add transactions of new block
        for(txid in transactionsNew) {
            //extract destination of prev transaction and
            //extract destination of this transaction should do the trick
            bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet)
            bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, vector<CTxDestination>& addressRet, int& nRequiredRet)
            //...
            //
        }

        pindex = pindex->pprev;
    }

}
