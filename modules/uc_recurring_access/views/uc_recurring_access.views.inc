<?php
// $Id$
/**
 * @file
 * Views 2 hooks and callback registries.
 */

/**
 * Implementation of hook_views_data().
 */
function uc_recurring_access_views_data() {
  $data['uc_recurring_access_content'] = array(
    'table' => array(
      'group' => t('Recurring Access'),
      'join' => array(
        'node' => array(
          'left_field' => 'nid',
          'field' => 'nid',
        ),
      ),
    ),
    'time_required' => array(
      'title' => t('Time Required to Access'),
      'help' => t('The total amount of time a user requires to have purchase'),
      'field' => array(
        'handler' => 'uc_recurring_access_handler_field_time',
        'click sortable' => TRUE,
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_numeric',
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_date',
      ),
    ),
    'access_type' => array(
      'title' => t('Type of access rule.'),
      'help' => t('Type of rule'),
      'field' => array(
        'handler' => 'views_handler_field',
      ),
      'filter' => array(
        'handler' => 'uc_recurring_access_handler_filter_access_type',
      ),
    ),
    'id' => array(
      'title' => t('Access Type ID'),
      'help' => t('The specific access type id'),
      'field' => array(
        'handler' => 'views_handler_field',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_numeric',
      ),
    ),
  );

  $data['uc_recurring_access_user'] = array(
    'table' => array(
      'group' => t('Recurring Access'),
      'join' => array(
        'node' => array(
          'table' => 'uc_recurring_access_user',
          'left_table' => 'uc_recurring_access_content',
          'left_field' => 'id',
          'field' => 'id',
          'extra' => 'uc_recurring_access_user.access_type = uc_recurring_access_content.access_type'
        ),
      ),
    ),
    'current_uid' => array(
      'real field' => 'uid',
      'title' => t('Current Uid'),
      'help' => t('Filter on current User ID.'),
      'filter' => array(
        'handler' => 'views_handler_filter_user_current',
      ),
    ),
    'uid' => array(
      'title' => t('Uid'),
      'help' => t('Filter on specific User ID.'),
      'filter' => array(
        'handler' => 'views_handler_filter_user_name',
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_user_uid',
        'numeric' => TRUE,
      ),
    ),
    'time_applied' => array(
      'title' => t('Time applied'),
      'help' => t('Time currently applied to the users account'),
      'field' => array(
        'handler' => 'uc_recurring_access_handler_field_time',
        'click sortable' => TRUE,
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_numeric',
      ),
    ),
    'has_access' => array(
      'title' => t('Has access'),
      'help' => t('Check if the user has access to the content'),
      'real field' => 'time_applied',
      'filter' => array(
        'handler' => 'uc_recurring_access_handler_filter_has_access',
        'label' => t('Has access to content'),
      ),
    ),
    'access_in' => array(
      'title' => t('Has access in'),
      'help' => t('Checkout how many seconds till user has access, e.g. find the content that will be available in a months time'),
      'real field' => 'time_applied',
      'field' => array(
        'handler' => 'uc_recurring_access_handler_field_time_in',
      ),
      'filter' => array(
        'handler' => 'uc_recurring_access_handler_filter_has_access_in',
        'label' => t('Has access to content in'),
      ),
    ),
  );

  $data['uc_recurring_access_user']['access_rid'] = array(
    'real field' => 'id',
    'title' => t('Rid'),
    'help' => t('Filter the view by role access rule. That is a recurring access rule which says the user must have purchases a product with a specific role.'),
    'filter' => array(
      'handler' => 'views_handler_filter_user_roles',
    ),
    'argument' => array(
      'handler' => 'uc_recurring_access_handler_argument_users_roles_rid',
      'numeric' => TRUE,
    ),
  );

  $data['uc_recurring_access_user']['access_nid'] = array(
    'real field' => 'id',
    'title' => t('Nid'),
    'help' => t('Filter the view by product access rule.'),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_node_nid',
      'numeric' => TRUE,
    ),
  );
  return $data;
}

/**
 * Implementation of hook_views_handlers().
 */
function uc_recurring_access_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'uc_recurring_access') . '/views',
    ),
    'handlers' => array(
      // fields
      'uc_recurring_access_handler_field_time' => array( 'parent' => 'views_handler_field',),
      'uc_recurring_access_handler_field_time_in' => array( 'parent' => 'views_handler_field',),
      // fiters
      'uc_recurring_access_handler_filter_access_type' => array('parent' => 'views_handler_filter_in_operator',),
      'uc_recurring_access_handler_filter_has_access' => array('parent' => 'views_handler_filter_boolean_operator',),
      'uc_recurring_access_handler_filter_has_access_in' => array('parent' => 'views_handler_filter_numeric',),
      // arguments
      'uc_recurring_access_handler_argument_users_roles_rid' => array('parent' => 'views_handler_argument_users_roles_rid',),
    ),
  );
}

