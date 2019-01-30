#include "claimforvote.hpp"

void claimforvote::reset()
{
  require_auth(_self);
  auto i = 0;
  auto itr = _claimer.begin();
  while (itr != _claimer.end())
  {
    i++;
    _claimer.erase(itr);
    itr = _claimer.begin();
  }
}

void claimforvote::claim(const account_name &user)
{
  require_auth(user);
  innerClaim(user);
}

void claimforvote::transfer(const account_name from, const account_name to, const asset quantity, const string memo)
{
  if (from == _self || to != _self)
  {
    return;
  }

  innerClaim(from);

  //return enu token
  action(permission_level{_self, N(active)}, N(enu.token), N(transfer), std::make_tuple(_self, from, quantity, string("give back")))
      .send();
}

void claimforvote::innerClaim(const account_name &user)
{
  /* get voter info */
  const auto &voter = _voters.get(user, "unable to find your vote info");
  enumivo_assert(voter.producers.size() == 1, "you can get rewards when only vote 1 producer");

  auto ct = now();

  auto claimer_itr = _claimer.find(user);
  if (claimer_itr != _claimer.end())
  {
    enumivo_assert(ct - claimer_itr->last_claim_time > sec_per_day, "already claimed rewards within past day");
    _claimer.modify(claimer_itr, 0, [&](auto &c) {
      c.last_claim_time = ct;
    });
  }
  else
  {
    claimer_itr = _claimer.emplace(_self, [&](auto &c) {
      c.claimer = user;
      c.last_claim_time = ct;
    });
  }

  /* assume token precision is 4 */
  auto rewards = asset(voter.last_vote_weight / 500000 / 100, TOKEN_SYMBOL);

  /* issue */
  action(permission_level{_self, N(active)}, TOKEN_CONTRACT, N(issue),
         std::make_tuple(user, rewards, std::string("Reward for vote")))
      .send();

  /* transfer */
  /* action(permission_level{_self, N(active)}, TOKEN_CONTRACT, N(transfer),
         std::make_tuple(_self, user, rewards, std::string("Reward for vote")))
      .send(); */
}
