<?php
// $Id: uc_views_handler_field_country.inc,v 1.1 2009/03/23 09:26:49 madsph Exp $

/**
 * @file
 * Views handler: Convert seconds into a Human friendly field
 */

/**
 * Return a time string.
 */
class uc_recurring_access_handler_filter_has_access extends views_handler_filter_boolean_operator {

  function query() {
    $this->ensure_my_table();
    $where = "$this->table_alias.$this->real_field ";
    if (empty($this->value)) {
      $where .= "<";
    }
    else {
      $where .= ">=";
    }
    $where .= " uc_recurring_access_content.time_required";

    $this->query->add_where($this->options['group'], $where);
  }

}
