<?php

// $Id: advanced_sphinx.pages.inc, v 1.0 2011/07/05 19:59:16 gagaga Exp $
/**
 * @file
 * Implementation of pages used to search using Sphinx search module.
 */

/**
 * Menu callback; presents the search form and/or search results.
 *
 * Search form submits with POST but redirects to GET. This way we can keep
 * the search query URL clean as a whistle:
 * http://www.example.com/search-path?keys=... (including keys and options)
 * @see advanced_sphinx_get_query_string()
 * group=atribute поля для группировки
  fil-atribute=znach,znach2,znach+n - поля для фильтрации
  sor-atribute=asc - сортировка
  sor-atribute2=desc -сортировка
  keys=keyword - ключвые слова
  view=v - просмотр без ключей
 */
function advanced_sphinx_search_page() {
  global $user;
  // Parse request and build search options structure.
  $search_options = advanced_sphinx_parse_request($_GET);
  variable_set('advanced_sphinx_keys', isset($_GET['keys']) ? $_GET['keys'] : NULL);

  // Execute search query and collect the results.
  if (isset($search_options['keys']) || (isset($search_options['keys']) && $search_options['view'])) {
    $search_results = advanced_sphinx_execute_query($search_options);
    $output = drupal_render(drupal_get_form('advanced_sphinx_search_form', $search_options));
    $output .= theme('advanced_sphinx_search_results', array('search_options' => $search_options,
      'search_results' => $search_results));
  }
  else {
    $output = drupal_render(drupal_get_form('advanced_sphinx_search_form', $search_options));
  }
  return $output;
}

/**
 * Record a warning to watchdog.
 *
 * If current user has 'administer advanced_sphinx' privilege,
 * then the warning is just sent to the user interface.
 *
 * @param string $message
 */
function advanced_sphinx_watchdog_warning($message) {
  if (user_access('administer advanced_sphinx')) {
    drupal_set_message($message, 'error');
  }
  else {
    watchdog('advanced_sphinx', $message, NULL, WATCHDOG_WARNING);
  }
}

/**
 * Render a search form.
 *
 * @param array $search_options
 *   The search string and options entered by the user.
 * @param boolean $advanced_options_collapsed
 *   TRUE to collapse Advanced search options fieldset.
 * @return array
 *   The search form.
 */
function advanced_sphinx_search_form($form, &$form_state, $search_options = array(), $advanced_options_collapsed = TRUE) {
  $form = array(
    '#action' => url(variable_get('advanced_sphinx_search_path', 'search-content')),
    '#attributes' => array(
      'class' => 'search-form'
    ),
  );
  $form['basic'] = array(
    '#type' => 'item',
    '#title' => t('Enter your keywords')
  );
  $form['basic']['inline'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>'
  );
  $form['basic']['inline']['keys'] = array(
    '#type' => 'textfield',
    '#title' => '',
    '#default_value' => isset($search_options['keys']) ? $search_options['keys'] : '',
    '#size' => 50,
    '#maxlength' => 255,
  );
  $form['basic']['inline']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Search')
  );

  return $form;
}

/**
 * Validate a search form submission.
 */
function advanced_sphinx_search_form_validate($form, &$form_state) {
  #drupal_set_message('<pre>'.print_r($search_options,1).'</pre>');
  if (empty($form_state['values']['keys'])) {
    form_set_error('keys', t('Enter search request in search field!'));
  }
}

/**
 * Process a search form submission.
 */
function advanced_sphinx_search_form_submit($form, &$form_state) {
  $search_options = advanced_sphinx_parse_request($form_state['values']);
  $query = advanced_sphinx_get_query_string($search_options);
  #drupal_set_message('<pre>' . print_r($search_options,1) . '</pre>');
  #drupal_set_message('<pre>' . print_r($query_string,1) . '</pre>');
  if (empty($query)) {
    form_set_error('keys', t('Please enter some keywords and/or other search options.'));
  }
  // Transform POST into a GET request.
  advanced_sphinx_goto_search($query);
}

function theme_advanced_sphinx_search_results($variables) {
  #drupal_set_message('<pre>'.print_r($search_options,1).'</pre>');
  #drupal_set_message('<pre>' . print_r($search_results,1) . '</pre>');
  // Display information about query execution.
  $output = '';
  extract($variables);
  // Display list of formatted search results.
  if (!isset($result_wrapper['result'])) {
    $result_wrapper = advanced_sphinx_get_items_result_main($search_options, $search_results);
  }
  $result_wrapper['stat'] = format_plural($search_results['total_found'], t('Found 1 result'), t('Found @count results')) . ' ' . t('on @seconds seconds.', array('@seconds' => round($search_results['time'], 3)));
  if (!empty($search_results['words'])) {
    $words = array();
    foreach ($search_results['words'] as $word => $word_data) {
      $words[] = '<em>' . check_plain($word) . '</em> (' . t('results: @docs, repeats: @hits', array('@docs' => (int) $word_data['docs'], '@hits' => (int) $word_data['hits'])) . ')';
    }
    $result_wrapper['word_stat'] = implode('; ', $words) . '.';
    $result_wrapper['sorted'] = theme('advanced_sphinx_sorted_links', advanced_sphinx_get_query_string($search_options));
  }

  $output .= theme('advanced_sphinx_result_wrapper', array('result' => $result_wrapper));

  // Display pager.
  $output .= advanced_sphinx_pager($search_results['total_available'], $search_options['results_per_page']);
  return $output;
}

function theme_advanced_sphinx_sorted_links($query, $sort = array()) {
  $sort_variant = array(
    'sor-relevance',
    'sor-created',
    'sor-changed'
  );
  $sort_title = array(
    t('Relevance'),
    t('Date'),
    t('Update')
  );
  $url = array();
  $sorted = '';
  foreach ($query as $sort => $sval) {
    $i = 0;
    if (strstr($sort, 'sor-') != FALSE) {
      unset($query[$sort]);
      while ($i < 3) {
        $query[$sort_variant[$i]] = 'DESC';
        $class = NULL;
        if ($sort == $sort_variant[$i]) {
          $class = 'class="sortnow"';
        }
        $url[] = '<a id="' . $sort_variant[$i] . '" href="' . url(variable_get('advanced_sphinx_search_path', 'search-content'), array('absolute' => TRUE, 'query' => $query)) . '" ' . $class . ' >' . $sort_title[$i] . '</a>';
        unset($query[$sort_variant[$i]]);
        $i++;
      }
      $key = str_replace("sor-", "", $sort);
      $sorted = $key;
    }
  }
  if ($sorted == '') {
    $i = 0;
    while ($i < 3) {
      $query[$sort_variant[$i]] = 'DESC';
      $class = NULL;
      if ('sor-relevance' == $sort_variant[$i]) {
        $class = 'class="sortnow"';
      }
      $url[] = '<a id="' . $sort_variant[$i] . '" href="' . url(variable_get('advanced_sphinx_search_path', 'search-content'), array('absolute' => TRUE, 'query' => $query)) . '" ' . $class . ' >' . $sort_title[$i] . '</a>';
      unset($query[$sort_variant[$i]]);
      $i++;
    }
  }
  return implode('  ', $url);
}

function advanced_sphinx_get_items_result_main($search_options, $search_results) {
  $items_result = array();
  $result_wrapper = array('result' => '');
  $page = isset($_GET['page']) ? $_GET['page'] : 0;

  foreach ($search_results['nodes'] as $item_id => $node) {
    $items_result['number'] = $item_id + 1 + $page * $search_options['results_per_page'];
    $title = (isset($search_results['titles'][$item_id]) ? $search_results['titles'][$item_id] : check_plain($node->title));
    if (variable_get('advanced_sphinx_logs', TRUE)) {
      $items_result['title'] = l($title, variable_get('advanced_sphinx_search_path', 'search-content') . '/service/' . $node->nid, array('html' => TRUE));
    }
    else {
      $items_result['title'] = l($title, 'node/' . $node->nid, array('html' => TRUE));
    }
    $items_result['excerpts'] = isset($search_results['excerpts'][$item_id]) ? '<p class="search-excerpt">' . $search_results['excerpts'][$item_id] . '</p>' . "\n" : '';
    //TODO D7
    /* if (module_exists('taxonomy') && is_array($node->taxonomy)) {
      $links = array();
      foreach ($node->taxonomy as $term) {
      $links[] = l($term->name, taxonomy_term_path($term));
      }
      $items_result['tags'] = implode(' - ', $links);
      } */
    $items_result['username'] = theme('username', array('account' => $node));
    $items_result['date'] = $node->created;
    if (property_exists($node, 'comment_count')) {
      $items_result['comment_count'] = $node->comment_count;
    }
    $result_wrapper['result'] .= theme('advanced_sphinx_items_result_main', array('result' => $items_result));
  }
  return $result_wrapper;
}
