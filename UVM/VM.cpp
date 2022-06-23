//
// Created by Kirill Zhukov on 31.05.2022.
//

#include "VM.h"
//#include "Opcodes.h"
#include "iostream"

VM::VM() {}
VM::~VM() {}

[[noreturn]] void VM::generate_block(Block *current, bool *lock) {
    std::cout << "Starting 'block generator'" << std::endl;
    add: {
        if (*lock) {
            goto add;
        } else {
            std::optional<std::string> op_block_height = unit::DB::get_block_height();
            std::string block_index = (op_block_height.has_value()) ? op_block_height.value() : R"({"index": 1})";
            nlohmann::json block_json = nlohmann::json::parse(block_index);
            uint64_t index = block_json["index"].get<uint64_t>() + 1;

//            std::map<std::string, std::string> map = {{"name", "unit"}, {"value", "0"}, {"bytecode", "null"}};
//            Transaction tx = Transaction("genesis", "g2px1", 0,  map, "0", 350000);
//            Transaction tx1 = Transaction("genesis", "teo", 0,  map, "0", 350000);
//            Transaction tx2 = Transaction("genesis", "sunaked", 0,  map, "0", 350000);
//            current->transactions.push_back(tx);
//            current->transactions.push_back(tx1);
//            current->transactions.push_back(tx2);

            for (Transaction &it : current->transactions) {
                it.setBlockId(index);
                if (!unit::DB::push_transaction(&it)) {
                    current->transactions.erase(std::remove(current->transactions.begin(), current->transactions.end(), it), current->transactions.end());
                }
            }

            current->setIndex(index);
            current->generate_hash();
            unit::DB::push_block(*current);
            goto generate;
        }
    };

    generate: {
        *current = Block(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10000)); // 1000 millisecond * 10 = 10 seconds
        goto add;
    };
}

[[noreturn]] void VM::run() {
    std::thread th(VM::generate_block, &currentblock, &block_lock);
    th.detach();
    std::thread server_th(Server::start_server, &transactions_deque);
    server_th.detach();
    loop: { // later need to check an instructions stack
        if (this->transactions_deque.empty()) {
            goto loop;
        } else {
            goto create_tx;
        }
    };

    create_tx: {
        this->block_lock = true;
        Transaction transaction = this->transactions_deque.front();
        currentblock.push_tx(transaction);
        this->transactions_deque.pop_front();
        this->block_lock = false;
        goto loop;
    };
}

void VM::popInstruction() {
    this->instructions.pop();
}

bool VM::pushInstruction(void *instruction) {
    try {
        this->instructions.push(instruction);
        return true;
    } catch (const std::exception& e){
        return false;
    }
}