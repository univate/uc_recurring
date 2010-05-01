<?php
// $Id$

/**
 * @file
 * Views argument
 */

/**
 *
 */
class uc_recurring_access_handler_argument_users_roles_rid extends views_handler_argument_users_roles_rid {

  function query() {
    parent::query();
    $this->query->add_where(0, "$this->table_alias.access_type = 'role'");
  }

}
