#include "claimforvote.hpp"

void claimforvote::claim(const account_name &user)
{
  require_auth(user);

  /* get voter info */
  const auto &voter = _voters.get(user, "unable to find your vote info");
  enumivo_assert(voter.producers.size() == 1, "you can get rewards when only vote 1 producer");

  auto ct = current_time();

  auto claimer_itr = _claimer.find(user);
  if (claimer_itr != _claimer.end())
  {
    enumivo_assert(ct - claimer_itr->last_claim_time > useconds_per_day, "already claimed rewards within past day");
    _claimer.modify(claimer_itr, 0, [&](auto &c) {
      c.last_claim_time = ct;
    });
  }
  else
  {
    claimer_itr = _claimer.emplace(user, [&](auto &c) {
      c.claimer = user;
      c.last_claim_time = ct;
    });
  }

  /* assume token precision is 4 */
  auto rewards = asset(voter.staked / 100, TOKEN_SYMBOL);

  action(permission_level{_self, N(active)}, TOKEN_CONTRACT, N(issue),
         std::make_tuple(user, rewards, std::string("Reward for vote")))
      .send();
}