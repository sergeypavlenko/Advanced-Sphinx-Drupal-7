<?php
// $Id: advanced_sphinx_result_wrapper.tpl.php, v 1.0 2011/07/05 19:59:16 gagaga Exp $
/**
 * @file advanced_sphinx_result_wrapper.tpl.php
 * Default theme implementation to wrap search result.
 *
 * Available variables:
 * - $result: Array with all data:
 *   - $result['stat']: General statistics search.
 *   - $result['word_stat']: Statistics search every word form.
 *   - $result['sorted']: Links to the sorting criteria.
 *   - $result['result']: List of result.
 */
?>
<?php print render($result['search_form']); ?>

<div class="search-stat">
  <?php print $result['stat']; ?>
  <?php if (isset($result['word_stat'])) { ?>
    <?php print $result['word_stat']; ?>
  <?php } ?>
</div>

<?php if (isset($result['sorted'])) { ?>
  <div
    class="search-sorted"><?php print t('Sort') . ': ' . $result['sorted']; ?></div>
<?php } ?>

<div id="advanced-search clear-block">
  <ul class="search-results">
    <?php print $result['result']; ?>
  </ul>
</div>

<?php if (isset($result['sorted'])) { ?>
  <div
    class="search-sorted"><?php print t('Sort') . ': ' . $result['sorted']; ?></div>
<?php } ?>

