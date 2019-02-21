#include <enulib/time.hpp>
#include <enulib/asset.hpp>
#include <enulib/singleton.hpp>
#include <enulib/transaction.hpp>
#include <enulib/crypto.h>
#include "enu.token.hpp"

#define TOKEN_CONTRACT N(t.red.test.v)
#define TOKEN_SYMBOL S(4, VOTE)
#define LOTTERY_POOL N(claimlottery)
#define LOTTERY_RATE_PERCENT 50

using namespace enumivo;
using namespace std;

const uint32_t sec_per_day = 24 * 3600;

class claimforvote : public contract
{
public:
  claimforvote(account_name self)
      : contract(self),
        _claimer(_self, _self),
        _global(_self, _self),
        _voters(N(enumivo), N(enumivo)){};

  // @abi action
  void claim(const account_name &user);

  // @abi action
  void check(const account_name &user);

  // @abi action
  void reset();

  void transfer(const account_name from, const account_name to, const asset quantity, const string memo);

private:
  void innerClaim(const account_name &user);

  uint64_t _next_id()
  {
    global_state _gstate;
    if (_global.exists())
    {
      _gstate = _global.get();
    }
    else
    {
      _gstate.next_id = 0;
    }
    _gstate.next_id = _gstate.next_id + 1;
    _global.set(_gstate, _self);
    return _gstate.next_id;
  }

  uint64_t _random(account_name user, uint64_t range)
  {
    auto mixd = tapos_block_prefix() * tapos_block_num() + user + current_time();
    const char *mixedChar = reinterpret_cast<const char *>(&mixd);
    checksum256 result;
    sha256((char *)mixedChar, sizeof(mixedChar), &result);
    uint64_t num1 = *(uint64_t *)(&result.hash[0]) + *(uint64_t *)(&result.hash[8]) * 10 + *(uint64_t *)(&result.hash[16]) * 100 + *(uint64_t *)(&result.hash[24]) * 1000;
    uint64_t random_num = (num1 % range);
    return random_num;
  }

  /* voter_info copy from system contract */
  struct voter_info
  {
    account_name owner = 0;
    account_name proxy = 0;
    std::vector<account_name> producers;
    int64_t staked = 0;
    double last_vote_weight = 0;
    double proxied_vote_weight = 0;
    bool is_proxy = 0;
    uint32_t reserved1 = 0;
    time reserved2 = 0;
    enumivo::asset reserved3;
    uint64_t primary_key() const { return owner; }

    ENULIB_SERIALIZE(voter_info, (owner)(proxy)(producers)(staked)(last_vote_weight)(proxied_vote_weight)(is_proxy)(reserved1)(reserved2)(reserved3))
  };

  typedef enumivo::multi_index<N(voters), voter_info> voters_table;
  voters_table _voters;

  // @abi table claimer i64
  struct claimer_info
  {
    account_name claimer;
    uint64_t last_claim_time = 0;
    uint64_t last_violation_time = 0;
    uint64_t primary_key() const { return claimer; }
    ENULIB_SERIALIZE(claimer_info, (claimer)(last_claim_time)(last_violation_time))
  };

  typedef enumivo::multi_index<N(claimer), claimer_info> claimer_index;
  claimer_index _claimer;

  // @abi table global i64
  struct global_state
  {
    uint64_t next_id;
    ENULIB_SERIALIZE(global_state, (next_id))
  };
  typedef enumivo::singleton<N(global), global_state> global_state_singleton;
  global_state_singleton _global;
};

#define ENUMIVO_ABI_EX(TYPE, MEMBERS)                                                                              \
  extern "C"                                                                                                       \
  {                                                                                                                \
    void apply(uint64_t receiver, uint64_t code, uint64_t action)                                                  \
    {                                                                                                              \
      if (action == N(onerror))                                                                                    \
      {                                                                                                            \
        enumivo_assert(code == N(enumivo), "onerror action's are only valid from the \"enumivo\" system account"); \
      }                                                                                                            \
      auto self = receiver;                                                                                        \
      if ((code == self && action != N(transfer)) || (code == N(enu.token) && action == N(transfer)))              \
      {                                                                                                            \
        TYPE thiscontract(self);                                                                                   \
        switch (action)                                                                                            \
        {                                                                                                          \
          ENUMIVO_API(TYPE, MEMBERS)                                                                               \
        }                                                                                                          \
      }                                                                                                            \
    }                                                                                                              \
  }

ENUMIVO_ABI_EX(claimforvote, (transfer)(claim)(reset)(check))