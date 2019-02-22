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
  _global.remove();
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

  if (memo != "donate")
  {

    innerClaim(from);

    //return enu token
    action(permission_level{_self, N(active)}, N(enu.token), N(transfer), std::make_tuple(_self, from, quantity, string("give back")))
        .send();
  }
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
    enumivo_assert(ct - claimer_itr->last_violation_time > (3 * sec_per_day), "you should wait 3 days after violation check");
    _claimer.modify(claimer_itr, 0, [&](auto &c) {
      c.last_claim_time = ct;
    });
  }
  else
  {
    claimer_itr = _claimer.emplace(_self, [&](auto &c) {
      c.claimer = user;
      c.last_claim_time = ct;
      c.last_violation_time = 0;
    });
  }

  /* assume token precision is 4 */
  auto rewards = asset(voter.last_vote_weight / 500000 / 100, TOKEN_SYMBOL);
  auto lottery_rewards = asset(voter.last_vote_weight / 500000 / 100 / 5, TOKEN_SYMBOL);

  /* issue */
  /* action(permission_level{_self, N(active)}, TOKEN_CONTRACT, N(issue),
         std::make_tuple(user, rewards, std::string("Reward for vote")))
      .send(); */

  /* transfer */
  action(permission_level{_self, N(active)}, TOKEN_CONTRACT, N(transfer),
         std::make_tuple(_self, user, rewards, std::string("Reward for vote")))
      .send();

  if (voter.staked >= LOTTERY_THRESHOLD)
  {
    /* transfer to lottery pool*/
    action(permission_level{_self, N(active)}, LOTTERY_TOKEN_CONTRACT, N(transfer),
           std::make_tuple(_self, LOTTERY_POOL, asset(LOTTERY_AMOUNT, LOTTERY_TOKEN_SYMBOL), std::string("Add reward to lottery pool")))
        .send();
  }

  /* send defer check action, cancel within 24 hours */
  enumivo::transaction txn{};
  txn.actions.emplace_back(
      enumivo::permission_level(_self, N(active)),
      _self,
      N(check),
      std::make_tuple(user));
  txn.delay_sec = _random(user, 60 * 60 * 24);
  txn.send(_next_id(), _self, false);
}

void claimforvote::check(const account_name &user)
{
  require_auth(_self);

  /* check user and next 10  */
  auto ct = now();
  auto claimer_itr = _claimer.find(user);
  if (claimer_itr != _claimer.end())
  {
    auto i = 0;
    while (i++ < 11)
    {
      auto &examinee = _voters.get(claimer_itr->claimer, "unable to find your vote info");
      if (examinee.producers.size() != 1)
      {
        _claimer.modify(claimer_itr, 0, [&](auto &c) {
          c.last_violation_time = ct;
        });
      }
      ++claimer_itr;
      if (claimer_itr == _claimer.end())
      {
        claimer_itr = _claimer.begin();
      }
    }
  }

  /* check lottery */
  const auto &voter = _voters.get(user, "unable to find your vote info");
  if (voter.staked >= LOTTERY_THRESHOLD)
  {
    auto lucky_num = _random(user, 100);
    print("lucky num: ", lucky_num);
    if (lucky_num > LOTTERY_RATE_PERCENT)
    {
      auto enu_token = enumivo::token(LOTTERY_TOKEN_CONTRACT);
      auto pool_balance = enu_token.get_balance(LOTTERY_POOL, symbol_type(LOTTERY_TOKEN_SYMBOL).name());
      print("lottery pool balance: ", pool_balance.amount);
      /* transfer lottery to winner*/
      action(permission_level{LOTTERY_POOL, N(active)}, LOTTERY_TOKEN_CONTRACT, N(transfer),
             std::make_tuple(LOTTERY_POOL, user, pool_balance, std::string("Claim lottery reward!")))
          .send();
    }
  }
}
