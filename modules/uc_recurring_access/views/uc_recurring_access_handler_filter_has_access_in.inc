<?php
// $Id$

/**
 * @file
 * Views handler
 */

/**
 *
 */
class uc_recurring_access_handler_filter_has_access_in extends views_handler_filter_numeric {

  function query() {
    $this->ensure_my_table();
    $field = '('. $this->query->tables['node']['uc_recurring_access_content']['alias'] .".time_required - $this->table_alias.$this->real_field)";

    $info = $this->operators();
    if (!empty($info[$this->operator]['method'])) {
      $this->{$info[$this->operator]['method']}($field);
    }
  }
}
