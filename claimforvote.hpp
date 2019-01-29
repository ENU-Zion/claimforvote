#include <enulib/time.hpp>
#include <enulib/asset.hpp>

#define TOKEN_CONTRACT N(t.red.test.v)
#define TOKEN_SYMBOL S(4, VOTE)

using namespace enumivo;
using namespace std;

const uint64_t useconds_per_day = 24 * 3600 * uint64_t(1000000);

class claimforvote : public contract
{
public:
  claimforvote(account_name self)
      : contract(self),
        _claimer(_self, _self),
        _voters(N(enumivo), N(enumivo)){};

  // @abi action
  void claim(const account_name &user);

private:
  struct voter_info
  {
    account_name owner = 0;              /// the voter
    account_name proxy = 0;              /// the proxy set by the voter, if any
    std::vector<account_name> producers; /// the producers approved by this voter if no proxy set
    int64_t staked = 0;

    /**
       *  Every time a vote is cast we must first "undo" the last vote weight, before casting the
       *  new vote weight.  Vote weight is calculated as:
       *
       *  stated.amount * 2 ^ ( weeks_since_launch/weeks_per_year)
       */
    double last_vote_weight = 0; /// the vote weight cast the last time the vote was updated

    /**
       * Total vote weight delegated to this voter.
       */
    double proxied_vote_weight = 0; /// the total vote weight delegated to this voter as a proxy
    bool is_proxy = 0;              /// whether the voter is a proxy for others

    uint32_t reserved1 = 0;
    time reserved2 = 0;
    enumivo::asset reserved3;

    uint64_t primary_key() const { return owner; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    ENULIB_SERIALIZE(voter_info, (owner)(proxy)(producers)(staked)(last_vote_weight)(proxied_vote_weight)(is_proxy)(reserved1)(reserved2)(reserved3))
  };

  typedef enumivo::multi_index<N(voters), voter_info> voters_table;
  voters_table _voters;

  // @abi table claimer i64
  struct claimer_info
  {
    account_name claimer;
    uint64_t last_claim_time = 0;
    uint64_t primary_key() const { return claimer; }
    ENULIB_SERIALIZE(claimer_info, (claimer)(last_claim_time))
  };

  typedef enumivo::multi_index<N(claimer), claimer_info> claimer_index;
  claimer_index _claimer;
};

ENUMIVO_ABI(claimforvote, (claim))