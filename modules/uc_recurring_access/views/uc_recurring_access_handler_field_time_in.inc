<?php
// $Id$

/**
 * @file
 * Views handler
 */

/**
 *
 */
class uc_recurring_access_handler_field_time_in extends views_handler_field {

  function query() {
    $this->ensure_my_table();
    // Add the field.
    $field = '('. $this->query->tables['node']['uc_recurring_access_content']['alias'] .".time_required - $this->table_alias.$this->real_field)";
    $this->field_alias = $this->query->add_field(NULL, $field);

    $this->add_additional_fields();
  }

  function render($values) {
    return theme('uc_recurring_time', $values->{$this->field_alias});
  }
}
