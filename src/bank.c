/***************************************************************************
 *  Dark Cloud copyright (C) 1998-2019                                     *
 ***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Strfeldt, Tom Madsen, and Katja Nyboe.    *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  ROM 2.4 improvements copyright (C) 1993-1998 Russ Taylor, Gabrielle    *
 *  Taylor and Brian Moore                                                 *
 *                                                                         *
 *  In order to use any part of this Diku Mud, you must comply with        *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt' as well as the ROM license.  In particular,   *
 *  you may not remove these copyright notices.                            *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 **************************************************************************/

/*
 * The Mythran Mud Economy Snippet Version 2 (used to be banking.c)
 *
 * Copyrights and rules for using the economy system:
 *
 *      The Mythran Mud Economy system was written by The Maniac, it was
 *      loosly based on the rather simple 'Ack!'s banking system'
 *
 *      If you use this code you must follow these rules.
 *              -Keep all the credits in the code.
 *              -Mail Maniac (v942346@si.hhs.nl) to say you use the code
 *              -Send a bug report, if you find 'it'
 *              -Credit me somewhere in your mud.
 *              -Follow the envy/merc/diku license
 *              -If you want to: send me some of your code
 *
 */

#if defined(macintosh)
    #include <types.h>
#else
    #include <sys/types.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "tables.h"

#define MAX_GOLD 999999

/*
 * Bank command to handle all transactions
 *
 * TODO - Can't withdraw more than you can carry
 */
void do_bank(CHAR_DATA * ch, char *argument)
{
  /*  The Mythran mud economy system (bank and trading)
   *
   *  Based on: Simple banking system. by -- Stephen --
   *
   *  The following changes and additions where made by the Maniac
   *  from Mythran Mud (v942346@si.hhs.nl)
   *
   *  History:
   *  05/18/1996:     Added the transfer option, enables chars to transfer
   *                  money from their account to other players' accounts
   *  05/18/1996:     Big bug detected, can deposit/withdraw/transfer
   *                  negative amounts (nice way to steal is
   *                  bank transfer -(lots of dogh)
   *                  Fixed it (thought this was better... -= Maniac =-)
   *  06/21/1996:     Fixed a bug in transfer (transfer to MOBS)
   *                  Moved balance from ch->balance to ch->pcdata->balance
   *  06/21/1996:     Started on the invest option, so players can invest
   *                  money in shares, using buy, sell and check
   *                  Finished version 1.0 releasing it monday 24/06/96
   *  06/24/1996:     Mythran Mud Economy System V1.0 released by Maniac
   *  03/01/2017:     Investing removed.
   *  03/03/2017:     Bank will only store gold, silver is more of an annoyance
   *                  for players.  The bank will however allow for silver to
   *                  be exchanged for gold which can then be deposited.
   *  03/04/2017:     Logging of deposits and withdrawls.
   *
   */

    CHAR_DATA *mob;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
    {
        send_to_char("Banking Services are only available to players.\r\n", ch);
        return;
    }

    // Check for a mob that is set as a banker
    for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
    {
        if (IS_NPC(mob) && IS_SET(mob->act, ACT_BANKER))
        {
            break;
        }
    }

    // No banker, sad trombone.
    if (mob == NULL)
    {
        send_to_char("You can't do that here, you need to find a bank.\r\n", ch);
        return;
    }

    // No arguments were entered, show the default options.
    if (argument[0] == '\0')
    {
        act("$N says '{gI offer the following banking services.{x'\r\n", ch, NULL, mob, TO_CHAR);

        send_to_char("  {_bank balance{x:                  {cDisplays your balance.{x\r\n", ch);
        send_to_char("  {_bank deposit # cointype{x:       {cDeposits a specific monetary type{x\r\n", ch);
        send_to_char("                                 {ce.g. bank deposit 39 gold{x\r\n", ch);
        send_to_char("  {_bank deposit all{x:              {cDeposits all coins{x\r\n", ch);
        send_to_char("  {_bank withdraw # cointype{x:      {cWithdraw a specific amount{x\r\n", ch);
        send_to_char("                                 {ce.g. bank withdraw 39 gold{x\r\n", ch);
        send_to_char("  {_bank exchange{x:                 {cExchanges silver for gold{x\r\n", ch);
        send_to_char("  {_bank transfer 100 gold player{x: {cTransfer coins to another persons account{x\r\n", ch);
        send_to_char("                                 {ce.g. bank transfer 100 gold blake{x\r\n", ch);
        return;
    }

    // Split all of the arguments off we'll need
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);

    // Now work out what to do...
    if (is_name(arg1, "balance"))
    {
        sendf(ch, "You have %s gold coin%s stored in the bank.\r\n", num_punct_long(ch->pcdata->bank_gold), ch->pcdata->bank_gold == 0 || ch->pcdata->bank_gold > 1 ? "s" : "");
        return;
    }

    // Handle deposits
    if (is_name(arg1, "deposit"))
    {
        if (is_number(arg2))
        {
            long amount = atoi(arg2);

            if (amount <= 0)
            {
                send_to_char("That is not a valid amount to deposit.\r\n", ch);
                return;
            }

            // Leaving this so we can deposit diamonds or eggs later.
            if (is_name(arg3, "gold"))
            {
                if (amount > ch->gold)
                {
                    send_to_char("You don't have that much gold.\r\n", ch);
                    return;
                }

                if ((amount + ch->pcdata->bank_gold) > MAX_GOLD)
                {
                    send_to_char("That deposit would push you over the maximum gold the bank will hold for you.\r\n", ch);
                    return;
                }

                // Deposit
                ch->gold -= amount;
                ch->pcdata->bank_gold += amount;

                // Show the balance
                sendf(ch, "You deposit %ld gold coin%s.  Your balance is now %ld gold coin%s.\r\n",
                    amount, amount == 0 || amount > 1 ? "s" : "",
                    ch->pcdata->bank_gold, ch->pcdata->bank_gold == 0 || ch->pcdata->bank_gold > 1 ? "s" : "");

                // Log the transaction
                log_f("%s deposits %ld gold coin%s.  Their balance is now %ld gold coin%s.",
                    ch->name, amount, amount == 0 || amount > 1 ? "s" : "",
                    ch->pcdata->bank_gold, ch->pcdata->bank_gold == 0 || ch->pcdata->bank_gold > 1 ? "s" : "");

                sprintf(buf, "[{GBANK{x] %s deposits %ld gold coin%s.  Their balance is now %ld gold coin%s.",
                    ch->name, amount, amount == 0 || amount > 1 ? "s" : "",
                    ch->pcdata->bank_gold, ch->pcdata->bank_gold == 0 || ch->pcdata->bank_gold > 1 ? "s" : "");

                wiznet(buf, ch, NULL, WIZ_BANK, 0, get_trust(ch));

            }
            else
            {
                send_to_char("What coin type did you want to deposit?\r\n", ch);
                return;
            }

            save_char_obj(ch);
            return;
        }
        else if (is_name(arg2, "all"))
        {
            if (ch->gold <= 0)
            {
                send_to_char("You don't have any worth to deposit.\r\n", ch);
                return;
            }

            long amount = ch->gold;

            if ((amount + ch->pcdata->bank_gold) > MAX_GOLD)
            {
                send_to_char("That deposit would push you over the maximum the bank will hold for you.\r\n", ch);
                return;
            }

            ch->pcdata->bank_gold += amount;
            ch->gold = 0;

            // Show the balance
            sendf(ch, "You deposit %ld gold coin%s.  Your balance is now %ld gold coin%s.\r\n",
                amount, amount == 0 || amount > 1 ? "s" : "",
                ch->pcdata->bank_gold, ch->pcdata->bank_gold == 0 || ch->pcdata->bank_gold > 1 ? "s" : "");

            // Log the transaction
            log_f("%s deposits %ld gold coin%s.  Their balance is now %ld gold coin%s.",
                ch->name, amount, amount == 0 || amount > 1 ? "s" : "",
                ch->pcdata->bank_gold, ch->pcdata->bank_gold == 0 || ch->pcdata->bank_gold > 1 ? "s" : "");

            sprintf(buf, "[{GBank{x] %s deposits %ld gold coin%s.  Their balance is now %ld gold coin%s.",
                ch->name, amount, amount == 0 || amount > 1 ? "s" : "",
                ch->pcdata->bank_gold, ch->pcdata->bank_gold == 0 || ch->pcdata->bank_gold > 1 ? "s" : "");

            wiznet(buf, ch, NULL, WIZ_BANK, 0, get_trust(ch));

            save_char_obj(ch);
            return;
        }
        else
        {
            send_to_char("What did you want to deposit?\r\n", ch);
            return;
        }

        return;
    }

    if (!str_prefix(arg1, "transfer"))
    {
        long amount;
        CHAR_DATA *victim;
        char wiz_buf[MSL];

        if (is_number(arg2))
        {
            amount = atoi(arg2);

            if (amount <= 0)
            {
                send_to_char("That is not a valid amount to transfer.\r\n", ch);
                return;
            }

            if (!(victim = get_char_world(ch, argument)))
            {
                send_to_char("They are not available for the transfer.\r\n", ch);
                return;
            }

            if (IS_NPC(victim))
            {
                send_to_char("You can only transfer money to other players.\r\n", ch);
                return;
            }

            if (ch == victim)
            {
                send_to_char("You cannot transfer funds to yourself.\r\n", ch);
                return;
            }

            if (is_name(arg3, "gold"))
            {
                if (amount > ch->pcdata->bank_gold)
                {
                    send_to_char("You have insufficient funds available to make a transfer of that size.\r\n", ch);
                    return;
                }

                ch->pcdata->bank_gold -= amount;
                victim->pcdata->bank_gold += amount;

                sendf(ch, "You transfer %ld gold to %s.\r\n", amount, victim->name);

                sprintf(buf, "[{GBANK{x] %s has transferred %ld gold to your account.\r\n", ch->name, amount);
                sprintf(wiz_buf, "[{GBANK{x] %s has transferred %ld gold to %s.", ch->name, amount, victim->name);
            }
            else
            {
                send_to_char("What did you want to transfer?\r\n", ch);
                return;
            }

            send_to_char(buf, victim);
            save_char_obj(ch);
            do_function(ch, &do_worth, "");
            save_char_obj(victim);

            wiznet(wiz_buf, ch, NULL, WIZ_BANK, 0, get_trust(ch));
            return;
        }
    }

    // Bank exchange code, silver to gold is all that we're currently supporting.
    if ((is_name(arg1, "exchange")) || (is_name(arg1, "change")))
    {
        if (ch->silver < 100)
        {
            send_to_char("You don't have enough silver to exchange for gold.\r\n", ch);
            return;
        }

        // Get the gold, the silver and the silver left over after the exchange.
        long gold = ch->silver / 100;
        long silver = ch->silver % 100;
        long silver_exchanged = ch->silver - silver;

        sendf(ch, "You exchange %ld silver for %ld gold.\r\n", silver_exchanged, gold);

        ch->gold += ch->silver / 100;
        ch->silver = ch->silver % 100;

        return;
    }

    // Withdraw code
    if (is_name(arg1, "withdraw"))
    {
        if (ch->pcdata->bank_gold <= 0)
        {
            ch->pcdata->bank_gold = 0;
            send_to_char("You don't have any funds to withdraw.\r\n", ch);
            return;
        }

        if (is_number(arg2))
        {
            long amount = atoi(arg2);

            if (amount <= 0)
            {
                send_to_char("That is not a valid amount to withdraw.\r\n", ch);
                return;
            }

            if (is_name(arg3, "gold coins"))
            {
                if (amount > ch->pcdata->bank_gold)
                {
                    send_to_char("You don't have that much gold in the bank.\r\n", ch);
                    return;
                }

                // Deduct the amount withdrawn
                ch->pcdata->bank_gold -= amount;
                ch->gold += amount;

                // Show the balance
                sendf(ch, "You withdraw %ld gold coin%s.  Your balance is now %ld gold coin%s.\r\n",
                    amount, amount == 0 || amount > 1 ? "s" : "",
                    ch->pcdata->bank_gold, ch->pcdata->bank_gold == 0 || ch->pcdata->bank_gold > 1 ? "s" : "");

                // Log the transaction
                log_f("%s withdraws %ld gold coin%s.  Their balance is now %ld gold coin%s.",
                    ch->name, amount, amount == 0 || amount > 1 ? "s" : "",
                    ch->pcdata->bank_gold, ch->pcdata->bank_gold == 0 || ch->pcdata->bank_gold > 1 ? "s" : "");

                sprintf(buf, "[{GBANK{x] %s withdraws %ld gold coin%s.  Their balance is now %ld gold coin%s.",
                    ch->name, amount, amount == 0 || amount > 1 ? "s" : "",
                    ch->pcdata->bank_gold, ch->pcdata->bank_gold == 0 || ch->pcdata->bank_gold > 1 ? "s" : "");

                wiznet(buf, ch, NULL, WIZ_BANK, 0, get_trust(ch));
            }
            else
            {
                send_to_char("What did you want to withdraw?\r\n", ch);
                return;
            }

            save_char_obj(ch);
            return;
        }
        else
        {
            send_to_char("What did you want to withdraw?\r\n", ch);
        }

        return;
    }

    // General instructions
    do_bank(ch, "");
    return;
}
